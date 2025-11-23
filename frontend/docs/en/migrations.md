---
outline: deep
description: C Web Framework database migration system. Creating, applying and rolling back migrations for PostgreSQL and MySQL via CLI.
---

# Database migrations

As the databases of applications that manage data are developed and maintained, the structures of the databases used evolve, as does the source code of the applications. For example, when developing an application, a new table may be needed in the future; already after the application is deployed in production mode (production), it may also be found that a certain index should be created to improve query performance; and so on. Due to the fact that changing the structure of a database often requires changing the source code, C Web Framework supports the so-called database migration capability, which allows you to track changes in databases using database migration terms, which are a version control system along with the source code.

C Web Framework provides a set of command line migration tools that allow you to:

* Create new migrations.
* Apply migrations.
* Cancel migrations.

All of these tools are available through the migrate program, which is compiled with cpdy. This section describes how to perform various tasks using these tools.

> Migrations can not only change the database schema, but also bring the data in line with the new schema.

## Create migrations

To create a new migration, run the following command:

```bash
migrate s1 create create_users_table /path/config.json
```

* s1 is the name of the server to which the migration is being applied.
* create - command to create a new migration.
* create_users_table - migration name.
* /path/config.json - path to the configuration file.

The above command will create a new handler in the `migrations/s1` directory with the file name `2023-07-01_20-00-00_create_users_table.c`. The file contains a code template to apply the migration and rollback:

```C
#include <stdlib.h>

#include "dbqueryf.h"
#include "dbresult.h"

int up(const char* dbid) {
    dbresult_t* result = dbqueryf(dbid, "");

    int res = dbresult_ok(result);

    dbresult_free(result);

    return res;
}
```

In the migration class, you must write code in the up() method when you make changes to the database structure. You can also write code in the down() method to undo the changes made by up() . The up method is called to update the database with the given migration, and the down() method is called to roll back the database changes. The following code shows how the migration class can be implemented to create the news table:

```C
#include <stdlib.h>

#include "dbqueryf.h"
#include "dbresult.h"

int up(const char* dbid) {
    dbresult_t* result = dbqueryf(dbid,
        "CREATE TABLE news"
        "("
            "id    bigserial     NOT NULL PRIMARY KEY, "
            "name  varchar(100)  NOT NULL DEFAULT '', "
            "text  text          NOT NULL DEFAULT '' "
        ")"
    );

    int res = dbresult_ok(result);

    dbresult_free(result);

    return res;
}
```

> Not all migrations are reversible. For example, if the up() method removes a row from a table, you may no longer be able to return that row to the down() method.


## Apply migrations

To update the database to the latest structure, you must apply all new migrations using the following command:

```bash
migrate s1 up /path/config.json

# specify all to explicitly apply all migrations
migrate s1 up all /path/config.json
```

* s1 is the name of the server to which the migration is being applied.
* up - command to apply the migration.
* all - the number of migrations to apply. In this case, everything.
* /path/config.json - path to the configuration file.

You can apply migrations individually by explicitly specifying the number of migrations to run per call to `migrate`.

```bash
migrate s1 up 1 /path/config.json;
migrate s1 up 3 /path/config.json;
```

For each migration that was successfully performed, this command will insert a row into the database table named migration, recording the successful migration. This allows the migration tool to identify which migrations have been applied and which have not.

> The migration tool will automatically create the migration table in the database specified in the configuration file.

Migrations can only be applied to specific databases. To do this, each database has a `migration` property in the configuration file. Set to `true` to run migrations for the selected database.

```json
"databases": {
    "postgresql": [
        {
            ...
            "migration": true
        }
    ],
    ...
}
```
