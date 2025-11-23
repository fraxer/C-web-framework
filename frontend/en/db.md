---
outline: deep
description: Setting up a database connection, accessing a database instance and executing a sql query
---

# Create a database connection

To access a database, you must first connect to it.

build a section in the config file. The configuration allows you to specify multiple servers.
New connections will be calculated per data server.

```json
"databases": {
    "postgresql": [
        {
            "ip": "127.0.0.1",
            "port": 5432,
            "dbname": "dbname",
            "user": "root",
            "password": "",
            "connection_timeout": 3,
            "migration": true
        }
    ],
    ...
}
```

Now you can execute queries using the database identifier:

```C
dbresult_t* result = dbquery("postgresql", "SELECT 1", NULL);
```

> The actual database connection will only be established the first time the `dbquery` method is called.

## Execute SQL queries

The following example shows how to retrieve data from a database:

```C
// handlers/indexpage.c
#include "http1.h"
#include "db.h"

void db(httpctx_t* ctx) {
    dbresult_t* result = dbquery("postgresql", "SELECT * FROM \"user\" LIMIT 3; SELECT * FROM \"news\";", NULL);

    if (!dbresult_ok(result)) {
        ctx->response->send_data(ctx, dbresult_error_message(result));
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

    ctx->response->send_data(ctx, "Done");

    failed:

    dbresult_free(result);
}
```

> Data is always retrieved as strings, even if the field type in the database is numeric.

PostgreSQL and MySQL support executing multiple statements in a single SQL query, as in the example above.

The `dbresult_query_next` method is used to move to the next result set.

## Parameter binding

When creating a command from an SQL query with parameters, you must use parameter binding:

```C
mparams_create_array(params,
    mparam_int(id, 10),
    mparam_text(name, "Alex")
);

dbresult_t* result = dbquery("postgresql",
    "SELECT * FROM \"user\" WHERE id > :id AND name <> :name",
    params
);

array_free(params);
```

> Parameter binding provides protection against SQL injection attacks.

## Execute Non-SELECT queries

INSERT, UPDATE, DELETE and other statements are executed in the same way through the `dbquery` method, but may not contain data to be displayed.
