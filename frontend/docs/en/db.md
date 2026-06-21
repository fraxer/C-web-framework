---
outline: deep
description: Working with databases in C Web Framework. PostgreSQL, MySQL, Redis, SQLite. Parameterized queries, ORM models, transactions, and SQL injection protection.
---

# Database

C Web Framework supports **PostgreSQL**, **MySQL**, **Redis**, and **SQLite** through a single unified API. The framework provides a connection pool (one per worker), parameterized queries, and SQL injection protection.

## Configuration

Connections are configured in `config.json` under the `databases` section. Each driver holds an array of hosts — each with its own `host_id`:

```json
"databases": {
    "postgresql": [{
        "host_id": "p1",
        "ip": "127.0.0.1",
        "port": 5432,
        "dbname": "mydb",
        "user": "dbuser",
        "password": "dbpass",
        "connection_timeout": 3,
        "schema": "public"
    }],
    "mysql": [{
        "host_id": "m1",
        "ip": "127.0.0.1",
        "port": 3306,
        "dbname": "mydb",
        "user": "dbuser",
        "password": "dbpass"
    }],
    "redis": [{
        "host_id": "r1",
        "ip": "127.0.0.1",
        "port": 6379,
        "dbindex": 0,
        "user": "",
        "password": ""
    }],
    "sqlite": [{
        "host_id": "s1",
        "path": "/var/data/app.db",
        "journal_mode": "WAL",
        "busy_timeout": 5000
    }]
}
```

::: tip Driver names
Constants are available in code: `POSTGRESQL`, `MYSQL`, `REDIS`, `SQLITE` (equal to the strings `"postgresql"`, `"mysql"`, `"redis"`, `"sqlite"`).
:::

## Database identifier (dbid)

The database a query targets is selected by the **dbid** — the first argument of every DB function. The format is:

```
<driver>.<host_id>
```

For example, `postgresql.p1` targets the PostgreSQL host with `host_id = "p1"`, and `sqlite.s1` targets the SQLite host `s1`.

::: tip Short form
Specifying only the driver (`"postgresql"`) selects the first configured host of that driver. Always prefer stating `host_id` explicitly — it is unambiguous and safe when several hosts exist.
:::

## Connection

A real connection is established **lazily** — on the first query against a given dbid — and is kept in the worker's connection pool. You never open or close a connection by hand; just call `dbquery`:

```c
#include "http.h"
#include "db.h"

void handler(httpctx_t* ctx) {
    dbresult_t* result = dbquery("postgresql.p1", "SELECT 1", NULL);

    if (!dbresult_ok(result)) {
        ctx->response->send_data(ctx->response, "Database not available");
        dbresult_free(result);
        return;
    }

    // Work with result...
    dbresult_free(result);
}
```

::: warning Memory management
Every `dbresult_t*` must be released with `dbresult_free(result)`. Structure your handler with `goto failed;` or early `return` so the result is freed on every path.
:::

## Executing queries

### Simple query

```c
void get_users(httpctx_t* ctx) {
    dbresult_t* result = dbquery("postgresql.p1",
        "SELECT id, name, email FROM \"user\" LIMIT 10", NULL);

    if (!dbresult_ok(result)) {
        ctx->response->send_data(ctx->response, dbresult_error(result));
        goto failed;
    }

    for (int row = 0; row < dbresult_query_rows(result); row++) {
        const db_table_cell_t* id   = dbresult_cell(result, row, 0);
        const db_table_cell_t* name = dbresult_cell(result, row, 1);
        printf("id=%s name=%.*s\n", id->value, (int)name->length, name->value);
    }

    ctx->response->send_data(ctx->response, "Done");

    failed:
    dbresult_free(result);
}
```

::: tip Values are always strings
Data is returned as strings (`char* value`) together with a `length`. Numbers, dates, and booleans also arrive as text — convert them as needed.
:::

### Reading a result

| Function | Purpose |
| --- | --- |
| `dbresult_ok(result)` | `1` if the query succeeded |
| `dbresult_error(result)` | Error text (if any) |
| `dbresult_query_rows(result)` | Number of rows in the current result set |
| `dbresult_query_cols(result)` | Number of columns |
| `dbresult_col_name(result, col)` | Column name by index |
| `dbresult_cell(result, row, col)` | Cell by row and column index |
| `dbresult_field(result, "name")` | Cell by column name (in the current row) |
| `dbresult_query_next(result)` | Advance to the next result set (multi-query) |
| `dbresult_insert_id(result)` | Last auto-increment `id` (insert) |
| `dbresult_free(result)` | Free the result |

The `db_table_cell_t` struct holds two fields: `size_t length` and `char* value`.

### Parameterized queries

Parameters protect against SQL injection: the value is never placed into the SQL text — it is passed separately and bound by the driver. Parameters are collected into an `array_t*` with helper macros:

```c
#include "http.h"
#include "db.h"
#include "query.h"

void get_user(httpctx_t* ctx) {
    int ok = 0;
    const int user_id = query_param_int(ctx->request->query_, "id", &ok);

    if (!ok) {
        ctx->response->send_default(ctx->response, 400);
        return;
    }

    array_t* params = array_create();
    mparams_fill_array(params,
        mparam_int(id, user_id)
    );

    dbresult_t* result = dbquery("postgresql.p1",
        "SELECT id, name, email FROM \"user\" WHERE id = :id",
        params
    );

    array_free(params);

    if (!dbresult_ok(result)) {
        ctx->response->send_default(ctx->response, 500);
        goto failed;
    }

    if (dbresult_query_rows(result) == 0) {
        ctx->response->send_default(ctx->response, 404);
        goto failed;
    }

    db_table_cell_t* name = dbresult_field(result, "name");
    ctx->response->send_data(ctx->response, name ? name->value : "");

    failed:
    dbresult_free(result);
}
```

#### Parameter syntax

| Notation | Purpose |
| --- | --- |
| `:name` | **Value** — bound as a query parameter (injection-safe) |
| `@name` | **Identifier** — a table/column name, escaped and inlined into the SQL text |

Use `:name` for data and `@name` only for dynamic schema/table/column names (e.g. `SELECT * FROM @table`).

#### Parameter types

The `mparam_*` macros create typed parameters. The parameter name (`#NAME`) is stringified and must match the placeholder in the SQL:

| Macro | Type |
| --- | --- |
| `mparam_bool(name, v)` | `bool` |
| `mparam_smallint(name, v)` | `smallint` |
| `mparam_int(name, v)` | `int` |
| `mparam_bigint(name, v)` | `bigint` |
| `mparam_float(name, v)` | `float` |
| `mparam_double(name, v)` | `double` |
| `mparam_decimal(name, v)` | `decimal` |
| `mparam_money(name, v)` | `money` (`double`) |
| `mparam_date(name, v)` | `date` |
| `mparam_time(name, v)` | `time` |
| `mparam_timestamp(name, v)` | `timestamp` |
| `mparam_timestamptz(name, v)` | `timestamp with time zone` |
| `mparam_json(name, v)` | `json` |
| `mparam_binary(name, v)` | `binary` / `blob` |
| `mparam_varchar(name, v)` | `varchar` |
| `mparam_char(name, v)` | `char` |
| `mparam_text(name, v)` | `text` |
| `mparam_enum(name, v, ...)` | `enum` |
| `mparam_array(name, v)` | `array` |

Several parameters are passed in a single statement:

```c
array_t* params = array_create();
mparams_fill_array(params,
    mparam_int(min_age, 18),
    mparam_int(max_age, 65),
    mparam_text(status, "active")
);

dbresult_t* result = dbquery("postgresql.p1",
    "SELECT * FROM \"user\" WHERE age BETWEEN :min_age AND :max_age AND status = :status",
    params
);

array_free(params);
```

### Multiple queries

PostgreSQL and MySQL support executing several queries in a single call. Each result set is iterated with `dbresult_query_next`:

```c
dbresult_t* result = dbquery("postgresql.p1",
    "SELECT * FROM \"user\" LIMIT 5; SELECT * FROM \"order\" LIMIT 5;", NULL);

if (dbresult_ok(result)) {
    do {
        for (int row = 0; row < dbresult_query_rows(result); row++) {
            for (int col = 0; col < dbresult_query_cols(result); col++) {
                const db_table_cell_t* field = dbresult_cell(result, row, col);
                printf("%s | ", field->value);
            }
            printf("\n");
        }
    } while (dbresult_query_next(result));
}

dbresult_free(result);
```

## INSERT, UPDATE, DELETE

```c
void create_user(httpctx_t* ctx) {
    char* name  = ctx->request->get_payloadf(ctx->request, "name");
    char* email = ctx->request->get_payloadf(ctx->request, "email");

    if (!name || !email) {
        ctx->response->send_default(ctx->response, 400);
        free(name); free(email);
        return;
    }

    array_t* params = array_create();
    mparams_fill_array(params,
        mparam_text(name, name),
        mparam_text(email, email)
    );

    dbresult_t* result = dbquery("postgresql.p1",
        "INSERT INTO \"user\" (name, email) VALUES (:name, :email) RETURNING id",
        params
    );

    array_free(params);

    if (!dbresult_ok(result)) {
        ctx->response->send_data(ctx->response, dbresult_error(result));
    } else {
        db_table_cell_t* id = dbresult_field(result, "id");
        ctx->response->send_data(ctx->response, id ? id->value : "0");
    }

    dbresult_free(result);
    free(name);
    free(email);
}
```

::: tip RETURNING and auto-increment
For PostgreSQL/SQLite use `RETURNING id`, or get the last inserted id via `dbresult_insert_id(result)`.
:::

## Query helpers

On top of `dbquery`, a set of helpers covers common table operations:

| Function | Purpose |
| --- | --- |
| `dbinsert(dbid, table, params)` | Insert a row |
| `dbupdate(dbid, table, set, where)` | Update by condition |
| `dbdelete(dbid, table, where)` | Delete by condition |
| `dbselect(dbid, table, columns, where)` | Select columns by condition |
| `dbexec(dbid, sql, params)` | Execute without returning rows (returns `int`) |
| `dbprepared(dbid, name, sql, params)` | Named prepared statement |
| `dbtable_exist(dbid, table)` | Check whether a table exists |

```c
// Insert
array_t* row = array_create();
mparams_fill_array(row,
    mparam_text(name, "John Doe"),
    mparam_text(email, "john@example.com")
);
dbresult_t* res = dbinsert("postgresql.p1", "\"user\"", row);
array_free(row);
dbresult_free(res);

// Select: columns and where are array_t*
array_t* columns = array_create();
array_push_back(columns, array_create_string("*"));
array_t* where = array_create();
mparams_fill_array(where, mparam_int(id, 42));

dbresult_t* sel = dbselect("postgresql.p1", "\"user\"", columns, where);
// ... process sel ...
array_free(columns);
array_free(where);
dbresult_free(sel);
```

### Prepared statements

`dbprepared` registers a prepared statement by name on the first call and reuses it on subsequent ones (per connection):

```c
array_t* params = array_create();
mparams_fill_array(params,
    mparam_int(id, user_id),
    mparam_text(email, "admin@example.com")
);

dbresult_t* result = dbprepared("postgresql.p1", "user_get",
    "SELECT id, name, email FROM \"user\" WHERE id = :id AND email = :email LIMIT 1",
    params);

array_free(params);
dbresult_free(result);
```

## Transactions

Transactions are managed with `dbbegin`, `dbcommit`, and `dbrollback`. The isolation level is set with the `transaction_level_e` enum: `READ_UNCOMMITTED`, `READ_COMMITTED`, `REPEATABLE_READ`, `SERIALIZABLE`.

```c
dbresult_t* b = dbbegin("postgresql.p1", READ_COMMITTED);
dbresult_free(b);

dbresult_t* r1 = dbquery("postgresql.p1",
    "UPDATE account SET balance = balance - 100 WHERE id = :id", debit);
dbresult_free(r1);

dbresult_t* r2 = dbquery("postgresql.p1",
    "UPDATE account SET balance = balance + 100 WHERE id = :id", credit);
dbresult_free(r2);

// Commit or roll back depending on the outcome
dbresult_free(dbcommit("postgresql.p1"));
// On error: dbresult_free(dbrollback("postgresql.p1"));
```

## Redis

Redis uses the same `dbquery` API — commands are passed as SQL-like text:

```c
void cache_example(httpctx_t* ctx) {
    // SET
    dbresult_t* set_result = dbquery("redis.r1", "SET mykey myvalue", NULL);
    dbresult_free(set_result);

    // GET
    dbresult_t* result = dbquery("redis.r1", "GET mykey", NULL);

    if (dbresult_ok(result)) {
        db_table_cell_t* value = dbresult_field(result, NULL);
        if (value) ctx->response->send_data(ctx->response, value->value);
    }

    dbresult_free(result);
}
```

## Models (ORM)

The framework provides an ORM layer based on schemas (`mschema_t`) and typed models. At the application level, wrappers are generated for each model — for `user`, for example: `user_instance`, `user_get`, `user_create`, `user_update`, `user_delete`, `user_free`.

### Creating

```c
#include "user.h"
#include "auth.h"

void create_user_example(httpctx_t* ctx) {
    user_t* user = user_instance();

    user_set_name(user, "John Doe");
    user_set_email(user, "john@example.com");

    // Password hash (PBKDF2-HMAC-SHA256)
    str_t* secret = generate_secret("password123");
    user_set_secret(user, str_get(secret));
    str_free(secret);

    if (!user_create(user)) {
        ctx->response->send_data(ctx->response, "Error creating user");
        user_free(user);
        return;
    }

    ctx->response->send_model(ctx->response, user,
                              display_fields("id", "email", "name"));

    user_free(user);
}
```

### Fetching a record

To find a record, build an array of parameters — parameter names become `WHERE ... = :name` clauses:

```c
void find_user(httpctx_t* ctx) {
    int ok = 0;
    const int id = query_param_int(ctx->request->query_, "id", &ok);

    if (!ok) {
        ctx->response->send_default(ctx->response, 400);
        return;
    }

    array_t* params = array_create();
    mparams_fill_array(params, mparam_int(id, id));

    user_t* user = user_get(params);
    array_free(params);

    if (!user) {
        ctx->response->send_default(ctx->response, 404);
        return;
    }

    ctx->response->send_model(ctx->response, user,
                              display_fields("id", "email", "name", "created_at"));

    user_free(user);
}
```

### Updating and deleting

```c
void update_user(httpctx_t* ctx) {
    int ok = 0;
    const int id = query_param_int(ctx->request->query_, "id", &ok);
    char* name = ctx->request->get_payloadf(ctx->request, "name");

    if (!ok || !name) {
        ctx->response->send_default(ctx->response, 400);
        free(name);
        return;
    }

    array_t* params = array_create();
    mparams_fill_array(params, mparam_int(id, id));
    user_t* user = user_get(params);
    array_free(params);

    if (!user) {
        ctx->response->send_default(ctx->response, 404);
        free(name);
        return;
    }

    user_set_name(user, name);

    if (!user_update(user)) {
        ctx->response->send_data(ctx->response, "Error updating user");
    } else {
        ctx->response->send_model(ctx->response, user,
                                  display_fields("id", "email", "name"));
    }

    user_free(user);
    free(name);
}
```

::: tip Low-level model functions
Generic functions are available directly: `model_create`, `model_update`, `model_delete`, `model_one`, `model_list`, `model_prepared_one`, `model_prepared_list`. Error diagnostics via `model_last_status()` (`MODEL_OK`, `MODEL_ERR_NOTFOUND`, `MODEL_ERR_DB`, `MODEL_ERR_PARAM`, `MODEL_ERR_ALLOC`) and `model_last_error()`.
:::

## See also

- [Prepared Statements](/en/prepared-statements) — in-depth on parameterization and SQL injection protection
- [Models (ORM)](/en/model) — defining models, schemas, and fields
- [Database migrations](/en/migrations) — the migration system
