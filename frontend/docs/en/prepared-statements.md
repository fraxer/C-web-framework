---
outline: deep
description: Prepared and parameterized queries in C Web Framework. :name and @name parameters, :list__ and @list__ lists, named prepared statements with dbprepared and model_prepared_one, SQL injection protection.
---

# Prepared Statements

Prepared (parameterized) queries separate values from the SQL text: data is passed separately and substituted by the driver through placeholders. This protects against SQL injection and lets the parsed query plan be reused.

C Web Framework provides three ways to run a query:

- `dbquery(dbid, sql, params)` — a one-shot parameterized query.
- `dbprepared(dbid, name, sql, params)` — a **named** prepared statement: on the first call with a given `name`, the statement is prepared and cached in the connection; subsequent calls reuse it.
- `model_prepared_one` / `model_prepared_list` — the same as `dbprepared`, but the result comes back as a typed ORM model (see [Models](/en/model) and the section below).

## Why use them

- ✅ **Security** — values are bound as parameters, never spliced into the SQL text.
- ✅ **Performance** — `dbprepared` caches the query plan by name within the connection.
- ✅ **Typing** — parameters are typed via the `mparam_*` macros.
- ✅ **Dynamic identifiers** — table/column names can be injected safely via `@name`.

## Parameter syntax

Parameters are marked in SQL with special prefixes:

| Notation | Purpose |
| --- | --- |
| `:name` | **Value** — bound as a placeholder (`$1`, `$2`, …). Injection-safe. |
| `@name` | **Identifier** — a table/column/schema name, escaped and inlined into the SQL text. |
| `:list__name` | **Value list** — expands to a comma-separated list of placeholders (`$1, $2, $3`). |
| `@list__name` | **Identifier list** — expands to a comma-separated list of escaped names. |

::: tip Two kinds of parameters
`:name` is for **data** (values). `@name` is for **names** of database objects (tables, columns, schemas). Never pass values through `@` — they are not bound and are not injection-safe.
:::

## Building parameters

Parameters are collected into an `array_t*` with the `mparam_*` macros. The parameter name (`#NAME`) is stringified and must match the placeholder in the SQL:

```c
array_t* params = array_create();
mparams_fill_array(params,
    mparam_int(id, user_id),
    mparam_text(status, "active")
);

// ... use params ...

array_free(params);
```

The full list of `mparam_*` types (`mparam_int`, `mparam_text`, `mparam_bool`, `mparam_double`, `mparam_array`, …) is in the [Database](/en/db#parameter-types) section.

::: tip mfield_def_* — defining a shape
The `mfield_def_int(name)`, `mfield_def_text(name)` macros create a typed parameter with a default value — handy for describing the shape of a statement. To execute with real data, use `mparam_*(name, value)`.
:::

## One-shot query — dbquery

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
        "SELECT id, name, email FROM \"user\" WHERE id = :id LIMIT 1",
        params
    );

    array_free(params);

    if (!dbresult_ok(result) || dbresult_query_rows(result) == 0) {
        ctx->response->send_default(ctx->response, 404);
        goto failed;
    }

    db_table_cell_t* name = dbresult_field(result, "name");
    ctx->response->send_data(ctx->response, name ? name->value : "");

    failed:
    dbresult_free(result);
}
```

## Named prepared statement — dbprepared

```c
dbresult_t* dbprepared(const char* dbid, const char* name, const char* sql, array_t* params);
```

`dbprepared` prepares the statement by name on the first call and reuses it on subsequent ones — the query plan is cached in the connection. The `sql` argument is only needed for the first preparation (it is ignored on later calls with the same `name`).

```c
#include "http.h"
#include "db.h"
#include "query.h"

void get_user_prepared(httpctx_t* ctx) {
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

    // The first call prepares "user_get_by_id" and caches it.
    // Later calls with the same name reuse the prepared statement.
    dbresult_t* result = dbprepared("postgresql.p1", "user_get_by_id",
        "SELECT id, name, email FROM \"user\" WHERE id = :id LIMIT 1",
        params
    );

    array_free(params);

    if (!dbresult_ok(result) || dbresult_query_rows(result) == 0) {
        ctx->response->send_default(ctx->response, 404);
        goto failed;
    }

    db_table_cell_t* name = dbresult_field(result, "name");
    ctx->response->send_data(ctx->response, name ? name->value : "");

    failed:
    dbresult_free(result);
}
```

::: tip When to use dbprepared
Use `dbprepared` for queries that run many times within a single connection — it saves re-parsing the plan. For one-off queries, `dbquery` is enough. The name must be unique within the connection.
:::

## CRUD examples

### Create (INSERT … RETURNING)

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

    dbresult_t* result = dbprepared("postgresql.p1", "user_create",
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

::: tip Getting the new row's id
Besides `RETURNING id`, the auto-increment key is available through `dbresult_insert_id(result)` — useful when the SQL returns no rows (e.g. MySQL/SQLite without `RETURNING`).
:::

### Update (UPDATE)

```c
void update_user(httpctx_t* ctx) {
    int ok = 0;
    const int user_id = query_param_int(ctx->request->query_, "id", &ok);
    char* name = ctx->request->get_payloadf(ctx->request, "name");
    if (!ok || !name) {
        ctx->response->send_default(ctx->response, 400);
        free(name);
        return;
    }

    array_t* params = array_create();
    mparams_fill_array(params,
        mparam_text(name, name),
        mparam_int(id, user_id)
    );

    dbresult_t* result = dbprepared("postgresql.p1", "user_update",
        "UPDATE \"user\" SET name = :name WHERE id = :id",
        params
    );

    array_free(params);

    if (!dbresult_ok(result)) {
        ctx->response->send_data(ctx->response, "Update failed");
    } else {
        ctx->response->send_data(ctx->response, "User updated");
    }

    dbresult_free(result);
    free(name);
}
```

### Delete (DELETE)

```c
void delete_user(httpctx_t* ctx) {
    int ok = 0;
    const int user_id = query_param_int(ctx->request->query_, "id", &ok);
    if (!ok) {
        ctx->response->send_default(ctx->response, 400);
        return;
    }

    array_t* params = array_create();
    mparams_fill_array(params, mparam_int(id, user_id));

    dbresult_t* result = dbprepared("postgresql.p1", "user_delete",
        "DELETE FROM \"user\" WHERE id = :id",
        params
    );

    array_free(params);
    dbresult_free(result);

    ctx->response->send_data(ctx->response, "Deleted");
}
```

### Search (LIKE)

```c
void search_users(httpctx_t* ctx) {
    int ok = 0;
    const char* q = query_param_char(ctx->request->query_, "q", &ok);
    if (!ok) {
        ctx->response->send_default(ctx->response, 400);
        return;
    }

    char pattern[512];
    snprintf(pattern, sizeof(pattern), "%%%s%%", q);

    array_t* params = array_create();
    mparams_fill_array(params, mparam_text(pattern, pattern));

    dbresult_t* result = dbquery("postgresql.p1",
        "SELECT id, name, email FROM \"user\" WHERE name ILIKE :pattern LIMIT 10",
        params
    );

    array_free(params);

    if (!dbresult_ok(result)) {
        ctx->response->send_data(ctx->response, "Search failed");
        goto failed;
    }

    for (int row = 0; row < dbresult_query_rows(result); row++) {
        const db_table_cell_t* name = dbresult_cell(result, row, 1);
        printf("%s\n", name->value);
    }

    ctx->response->send_data(ctx->response, "Done");

    failed:
    dbresult_free(result);
}
```

## Dynamic identifiers and lists

Sometimes the table/column name or a set of values is not known in advance. For this, use `@name`, `@list__name`, and `:list__name`.

### Dynamic names (`@name`, `@list__name`)

`@name` injects an escaped identifier and `@list__name` injects a comma-separated list of identifiers. The source is an array parameter (`mparam_array`):

```c
array_t* fields = array_create_strings("id", "name", "email");
array_t* params = array_create();
mparams_fill_array(params,
    mparam_array(fields, fields),   // @list__fields -> "id", "name", "email"
    mparam_text(table, "user")      // @table        -> "user"
);

dbresult_t* result = dbquery("postgresql.p1",
    "SELECT @list__fields FROM @table WHERE id = :id",
    params
);

array_free(params);
// mparam_array does not copy the array: params owns fields.
// Free only params — do NOT call array_free(fields) separately.
dbresult_free(result);
```

### Value list (`:list__name`)

`:list__name` expands to a list of placeholders — handy for `IN (...)`:

```c
array_t* id_arr = array_create_from_ints((int[]){ 1, 5, 9 }, 3);

array_t* params = array_create();
mparams_fill_array(params, mparam_array(id, id_arr));

dbresult_t* result = dbquery("postgresql.p1",
    "SELECT * FROM \"user\" WHERE id IN (:list__id)",
    params
);
// Expands to: SELECT * FROM "user" WHERE id IN ($1, $2, $3)

array_free(params);
// params owns id_arr — no separate array_free(id_arr) is needed.
dbresult_free(result);
```

::: warning Array ownership
`mparam_array(name, arr)` does **not** copy the array — ownership moves to `params`. Free only `params` via `array_free(params)`. Do not free the inner array separately — that would double-free.
:::

## SQL injection protection

`:name` and `:list__name` parameters are bound by the driver as values, not spliced into the SQL text, so malicious input stays data and cannot change the query structure:

```c
// Dangerous — the value is spliced into the SQL text (injection!):
// snprintf(sql, "SELECT * FROM \"user\" WHERE email = '%s'", user_input);

// Safe — the value is bound as a parameter:
array_t* params = array_create();
mparams_fill_array(params, mparam_text(email, user_input));
dbresult_t* result = dbquery("postgresql.p1",
    "SELECT * FROM \"user\" WHERE email = :email", params);
array_free(params);
```

::: tip The rule
All values go through `:name`. All dynamic object names go through `@name` (escaped as identifiers). Never assemble SQL from user input with string functions.
:::

## Prepared statements and models (ORM)

`dbquery` / `dbprepared` return "raw" cells (`db_table_cell_t`). When the application already defines an ORM model (see [Models](/en/model)), it is more convenient to get a typed object back — `model_prepared_one` / `model_prepared_list` do that:

```c
#include "http.h"
#include "db.h"
#include "query.h"
#include "model.h"
#include "user.h"   // app: user_instance, user_t

void get_user_model(httpctx_t* ctx) {
    int ok = 0;
    const int user_id = query_param_int(ctx->request->query_, "id", &ok);
    if (!ok) {
        ctx->response->send_default(ctx->response, 400);
        return;
    }

    array_t* params = array_create();
    mparams_fill_array(params, mparam_int(id, user_id));

    // The first call prepares "user_get_by_id" and caches it in the connection.
    // The result is a typed model user_t* (or NULL on error / not found).
    user_t* user = model_prepared_one("postgresql.p1", user_instance,
        "user_get_by_id",
        "SELECT id, name, email FROM \"user\" WHERE id = :id LIMIT 1",
        params
    );

    array_free(params);

    if (user == NULL) {
        // The reason is available via model_last_status():
        //   MODEL_ERR_NOTFOUND -> 404, MODEL_ERR_DB -> 500.
        ctx->response->send_default(ctx->response,
            model_last_status() == MODEL_ERR_NOTFOUND ? 404 : 500);
        return;
    }

    ctx->response->send_model(ctx->response, user,
        display_fields("id", "email", "name"));

    model_free(user);
}
```

For multiple rows, `model_prepared_list` returns an `array_t*` of models, sent with `send_models`:

```c
array_t* users = model_prepared_list("postgresql.p1", user_instance,
    "users_active",
    "SELECT id, name, email FROM \"user\" WHERE status = :status",
    params
);

if (users != NULL) {
    ctx->response->send_models(ctx->response, users,
        display_fields("id", "email", "name"));
    array_free(users);
}
```

::: tip When NULL is not an error
`model_prepared_one` returns `NULL` both for "not found" and for a DB error. Tell them apart with `model_last_status()` (`MODEL_OK`, `MODEL_ERR_NOTFOUND`, `MODEL_ERR_DB`, `MODEL_ERR_PARAM`, `MODEL_ERR_ALLOC`); the error text is in `model_last_error()`.
:::

## See also

- [Database](/en/db) — configuration, `dbquery`, parameter types, transactions
- [Models (ORM)](/en/model) — `model_prepared_one` / `model_prepared_list` for typed models
- [Database migrations](/en/migrations) — the migration system
