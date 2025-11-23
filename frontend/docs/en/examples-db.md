---
outline: deep
description: Database examples
---

# Database examples

## PostgreSQL or MySQL

```C
// handlers/indexpage.c
#include "http.h"
#include "db.h"

void db(httpctx_t* ctx) {
    dbresult_t* result = dbquery("postgresql", "SELECT * FROM \"user\" LIMIT 3; SELECT * FROM \"news\";", NULL);

    if (!dbresult_ok(result)) {
        ctx->response->send_data(ctx->response, dbresult_error_message(result));
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

```C
void db_field(httpctx_t* ctx) {
    dbresult_t* result = dbquery("mysql", "select * from site limit 1;", NULL);

    if (!dbresult_ok(result)) {
        ctx->response->send_data(ctx->response, dbresult_error_message(result));
        goto failed;
    }

    if (dbresult_query_rows(result) == 0) {
        ctx->response->send_data(ctx->response, "No results");
        goto failed;
    }

    db_table_cell_t* field = dbresult_field(result, "domain");

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

```C
void db_redis(httpctx_t* ctx) {
    // dbresult_t* result = dbquery("redis", "SET testkey testvalue", NULL);
    dbresult_t* result = dbquery("redis", "GET testkey", NULL);

    if (!dbresult_ok(result)) {
        ctx->response->send_data(ctx->response, dbresult_error_message(result));
        goto failed;
    }

    const db_table_cell_t* field = dbresult_field(result, NULL);

    ctx->response->send_data(ctx->response, field->value);

    failed:

    dbresult_free(result);
}
```

## Parameterized queries with URL parameters

```C
#include "http.h"
#include "db.h"
#include "query.h"

void get_user(httpctx_t* ctx) {
    int ok = 0;
    const char* user_id = query_param_char(ctx->request, "id", &ok);

    if (!ok) {
        ctx->response->send_data(ctx->response, "Missing id parameter");
        return;
    }

    mparams_create_array(params,
        mparam_int(id, atoi(user_id))
    );

    dbresult_t* result = dbquery("postgresql",
        "SELECT id, name, email FROM \"user\" WHERE id = :id",
        params
    );

    array_free(params);

    if (!dbresult_ok(result)) {
        ctx->response->send_data(ctx->response, "Query failed");
        dbresult_free(result);
        return;
    }

    if (dbresult_query_rows(result) == 0) {
        ctx->response->send_data(ctx->response, "User not found");
        dbresult_free(result);
        return;
    }

    db_table_cell_t* name_field = dbresult_field(result, "name");
    db_table_cell_t* email_field = dbresult_field(result, "email");

    char response[512];
    snprintf(response, sizeof(response), "User: %s, Email: %s",
             name_field ? name_field->value : "N/A",
             email_field ? email_field->value : "N/A");

    ctx->response->send_data(ctx->response, response);
    dbresult_free(result);
}
```

## Insert data with parameters

```C
#include "http.h"
#include "db.h"

void create_user(httpctx_t* ctx) {
    char* name = ctx->request->get_payloadf(ctx->request, "name");
    char* email = ctx->request->get_payloadf(ctx->request, "email");

    if (!name || !email) {
        ctx->response->send_data(ctx->response, "Missing name or email");
        if (name) free(name);
        if (email) free(email);
        return;
    }

    mparams_create_array(params,
        mparam_text(name, name),
        mparam_text(email, email)
    );

    dbresult_t* result = dbquery("postgresql",
        "INSERT INTO \"user\" (name, email) VALUES (:name, :email) RETURNING id",
        params
    );

    array_free(params);

    if (!dbresult_ok(result)) {
        ctx->response->send_data(ctx->response, "Insert failed");
        dbresult_free(result);
        free(name);
        free(email);
        return;
    }

    db_table_cell_t* id_field = dbresult_field(result, "id");
    char response[256];
    snprintf(response, sizeof(response), "User created with id: %s",
             id_field ? id_field->value : "unknown");

    ctx->response->send_data(ctx->response, response);
    dbresult_free(result);
    free(name);
    free(email);
}
```

## Update data

```C
#include "http.h"
#include "db.h"
#include "query.h"

void update_user(httpctx_t* ctx) {
    int ok = 0;
    const char* user_id = query_param_char(ctx->request, "id", &ok);

    if (!ok) {
        ctx->response->send_data(ctx->response, "Missing id parameter");
        return;
    }

    char* name = ctx->request->get_payloadf(ctx->request, "name");

    if (!name) {
        ctx->response->send_data(ctx->response, "Missing name field");
        return;
    }

    mparams_create_array(params,
        mparam_text(name, name),
        mparam_int(id, atoi(user_id))
    );

    dbresult_t* result = dbquery("postgresql",
        "UPDATE \"user\" SET name = :name WHERE id = :id",
        params
    );

    array_free(params);

    if (!dbresult_ok(result)) {
        ctx->response->send_data(ctx->response, "Update failed");
        dbresult_free(result);
        free(name);
        return;
    }

    ctx->response->send_data(ctx->response, "User updated");
    dbresult_free(result);
    free(name);
}
```

## Search with filter

```C
#include "http.h"
#include "db.h"
#include "query.h"

void search_users(httpctx_t* ctx) {
    int ok = 0;
    const char* search_term = query_param_char(ctx->request, "q", &ok);

    if (!ok) {
        ctx->response->send_data(ctx->response, "Missing search parameter");
        return;
    }

    char search_pattern[512];
    snprintf(search_pattern, sizeof(search_pattern), "%%%s%%", search_term);

    mparams_create_array(params,
        mparam_text(pattern, search_pattern)
    );

    dbresult_t* result = dbquery("postgresql",
        "SELECT id, name, email FROM \"user\" WHERE name ILIKE :pattern LIMIT 10",
        params
    );

    array_free(params);

    if (!dbresult_ok(result)) {
        ctx->response->send_data(ctx->response, "Search failed");
        dbresult_free(result);
        return;
    }

    if (dbresult_query_rows(result) == 0) {
        ctx->response->send_data(ctx->response, "No users found");
        dbresult_free(result);
        return;
    }

    char response[2048] = "Found users: ";
    for (int row = 0; row < dbresult_query_rows(result); row++) {
        db_table_cell_t* id = dbresult_cell(result, row, 0);
        db_table_cell_t* name = dbresult_cell(result, row, 1);

        char line[256];
        snprintf(line, sizeof(line), "[%s] %s | ", id->value, name->value);
        strncat(response, line, sizeof(response) - strlen(response) - 1);
    }

    ctx->response->send_data(ctx->response, response);
    dbresult_free(result);
}
```
