---
outline: deep
description: Database examples
---

# Database examples

Ready-to-use query examples for PostgreSQL, MySQL, Redis, and SQLite through the
unified `dbquery` API. For the full function reference, see [Database](/en/db).

## PostgreSQL or MySQL

Iterating several result sets returned by a single multi-query call:

```c
// handlers/indexpage.c
#include "http.h"
#include "db.h"

void db(httpctx_t* ctx) {
    dbresult_t* result = dbquery("postgresql.p1",
        "SELECT * FROM \"user\" LIMIT 3; SELECT * FROM \"news\";", NULL);

    if (!dbresult_ok(result)) {
        ctx->response->send_data(ctx->response, dbresult_error(result));
        goto failed;
    }

    do {
        for (int row = 0; row < dbresult_query_rows(result); row++) {
            for (int col = 0; col < dbresult_query_cols(result); col++) {
                const db_table_cell_t* field = dbresult_cell(result, row, col);
                printf("%s | ", field->value);
            }
            printf("\n");
        }
        printf("\n");

        dbresult_row_first(result);
        dbresult_col_first(result);
    } while (dbresult_query_next(result));

    ctx->response->send_data(ctx->response, "Done");

    failed:

    dbresult_free(result);
}
```

Reading a single field by column name:

```c
void db_field(httpctx_t* ctx) {
    dbresult_t* result = dbquery("mysql.m1", "select * from site limit 1;", NULL);

    if (!dbresult_ok(result)) {
        ctx->response->send_data(ctx->response, dbresult_error(result));
        goto failed;
    }

    if (dbresult_query_rows(result) == 0) {
        ctx->response->send_default(ctx->response, 404);
        goto failed;
    }

    const db_table_cell_t* field = dbresult_field(result, "domain");

    if (!field) {
        ctx->response->send_data(ctx->response, "Field domain not found");
        goto failed;
    }

    ctx->response->send_data(ctx->response, field->value);

    failed:

    dbresult_free(result);
}
```

## Redis

Redis uses the same `dbquery` API — commands are passed as text:

```c
void db_redis(httpctx_t* ctx) {
    dbresult_t* set_result = dbquery("redis.r1", "SET testkey testvalue", NULL);
    dbresult_free(set_result);

    dbresult_t* result = dbquery("redis.r1", "GET testkey", NULL);

    if (!dbresult_ok(result)) {
        ctx->response->send_data(ctx->response, dbresult_error(result));
        goto failed;
    }

    const db_table_cell_t* field = dbresult_field(result, NULL);
    ctx->response->send_data(ctx->response, field ? field->value : "(nil)");

    failed:

    dbresult_free(result);
}
```

## Parameterized query with a URL parameter

Numeric parameters are best read already typed with `query_param_int` — no manual
`atoi` required:

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

    const db_table_cell_t* name_field  = dbresult_field(result, "name");
    const db_table_cell_t* email_field = dbresult_field(result, "email");

    char response[512];
    snprintf(response, sizeof(response), "User: %s, Email: %s",
             name_field ? name_field->value : "N/A",
             email_field ? email_field->value : "N/A");

    ctx->response->send_data(ctx->response, response);

    failed:

    dbresult_free(result);
}
```

## Insert data with parameters

```c
#include "http.h"
#include "db.h"

void create_user(httpctx_t* ctx) {
    char* name  = ctx->request->get_payloadf(ctx->request, "name");
    char* email = ctx->request->get_payloadf(ctx->request, "email");

    if (!name || !email) {
        ctx->response->send_default(ctx->response, 400);
        free(name);
        free(email);
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
        const db_table_cell_t* id_field = dbresult_field(result, "id");
        char response[256];
        snprintf(response, sizeof(response), "User created with id: %s",
                 id_field ? id_field->value : "unknown");
        ctx->response->send_data(ctx->response, response);
    }

    dbresult_free(result);
    free(name);
    free(email);
}
```

## Update data

```c
#include "http.h"
#include "db.h"
#include "query.h"

void update_user(httpctx_t* ctx) {
    int ok = 0;
    const int user_id = query_param_int(ctx->request->query_, "id", &ok);

    if (!ok) {
        ctx->response->send_default(ctx->response, 400);
        return;
    }

    char* name = ctx->request->get_payloadf(ctx->request, "name");

    if (!name) {
        ctx->response->send_default(ctx->response, 400);
        return;
    }

    array_t* params = array_create();
    mparams_fill_array(params,
        mparam_text(name, name),
        mparam_int(id, user_id)
    );

    dbresult_t* result = dbquery("postgresql.p1",
        "UPDATE \"user\" SET name = :name WHERE id = :id",
        params
    );

    array_free(params);

    if (!dbresult_ok(result)) {
        ctx->response->send_data(ctx->response, dbresult_error(result));
    } else {
        ctx->response->send_data(ctx->response, "User updated");
    }

    dbresult_free(result);
    free(name);
}
```

## Search with filter

String parameters are read with `query_param_char`. The `%term%` pattern is
passed as the value of the `:pattern` parameter rather than inlined into the SQL,
which keeps the query injection-safe:

```c
#include "http.h"
#include "db.h"
#include "query.h"

void search_users(httpctx_t* ctx) {
    int ok = 0;
    const char* search_term = query_param_char(ctx->request->query_, "q", &ok);

    if (!ok || !search_term) {
        ctx->response->send_default(ctx->response, 400);
        return;
    }

    char search_pattern[512];
    snprintf(search_pattern, sizeof(search_pattern), "%%%s%%", search_term);

    array_t* params = array_create();
    mparams_fill_array(params,
        mparam_text(pattern, search_pattern)
    );

    dbresult_t* result = dbquery("postgresql.p1",
        "SELECT id, name, email FROM \"user\" WHERE name ILIKE :pattern LIMIT 10",
        params
    );

    array_free(params);

    if (!dbresult_ok(result)) {
        ctx->response->send_data(ctx->response, dbresult_error(result));
        goto failed;
    }

    if (dbresult_query_rows(result) == 0) {
        ctx->response->send_default(ctx->response, 404);
        goto failed;
    }

    char response[2048] = "Found users: ";
    for (int row = 0; row < dbresult_query_rows(result); row++) {
        const db_table_cell_t* id   = dbresult_cell(result, row, 0);
        const db_table_cell_t* name = dbresult_cell(result, row, 1);

        char line[256];
        snprintf(line, sizeof(line), "[%s] %s | ",
                 id ? id->value : "?",
                 name ? name->value : "?");
        strncat(response, line, sizeof(response) - strlen(response) - 1);
    }

    ctx->response->send_data(ctx->response, response);

    failed:

    dbresult_free(result);
}
```

## Transactions

`dbbegin` / `dbcommit` / `dbrollback` drive a transaction. The isolation level is
set with the `transaction_level_e` enum (`READ_COMMITTED`, and so on). Roll back
if any statement fails:

```c
#include "http.h"
#include "db.h"
#include "query.h"

void transfer(httpctx_t* ctx) {
    int ok = 0;
    const int from_id = query_param_int(ctx->request->query_, "from", &ok);
    if (!ok) {
        ctx->response->send_default(ctx->response, 400);
        return;
    }
    const int to_id = query_param_int(ctx->request->query_, "to", &ok);
    if (!ok) {
        ctx->response->send_default(ctx->response, 400);
        return;
    }

    dbresult_free(dbbegin("postgresql.p1", READ_COMMITTED));

    array_t* debit = array_create();
    mparams_fill_array(debit, mparam_int(id, from_id));
    dbresult_t* r1 = dbquery("postgresql.p1",
        "UPDATE account SET balance = balance - 100 WHERE id = :id", debit);
    array_free(debit);

    if (!dbresult_ok(r1)) {
        dbresult_free(r1);
        goto rollback;
    }
    dbresult_free(r1);

    array_t* credit = array_create();
    mparams_fill_array(credit, mparam_int(id, to_id));
    dbresult_t* r2 = dbquery("postgresql.p1",
        "UPDATE account SET balance = balance + 100 WHERE id = :id", credit);
    array_free(credit);

    if (!dbresult_ok(r2)) {
        dbresult_free(r2);
        goto rollback;
    }
    dbresult_free(r2);

    dbresult_free(dbcommit("postgresql.p1"));
    ctx->response->send_data(ctx->response, "Transfer completed");
    return;

    rollback:

    dbresult_free(dbrollback("postgresql.p1"));
    ctx->response->send_default(ctx->response, 500);
}
```
