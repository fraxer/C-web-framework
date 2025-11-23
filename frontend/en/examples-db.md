---
outline: deep
description: Database examples
---

# Database examples

## PostgreSQL or MySQL

```C
// handlers/indexpage.c
#include "http1.h"
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
