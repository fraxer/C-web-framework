---
outline: deep
description: C Web Framework database migration system. Creating and applying migrations for PostgreSQL, MySQL, Redis, and SQLite via CLI.
---

# Database migrations

Migrations let you version the database schema together with the application source code. Since a schema change almost always requires a matching code change, the framework treats migrations as part of the project's version control.

Typical scenarios: add a new table during development, create an index to speed up queries after going to production, rename a column, or transform data to match a new schema.

C Web Framework ships a `migrate` CLI utility, compiled alongside `cpdy`, that lets you:

* Create new migrations.
* Apply migrations.

> Migrations can not only change the database schema but also transform the data itself — the full [database layer](./db.md) is available inside a migration.

## How it works

Each migration is a C source file compiled into a shared library (`.so`) and loaded by the `migrate` utility at runtime. The file defines a single function:

```c
int up(const char* dbid);
```

The `dbid` parameter is a database identifier from `config.json` (e.g. `postgresql.p1`). The function must run SQL statements through [dbquery](./db.md) and return `1` on success or `0` on failure. A successfully applied migration is recorded in a `migration` bookkeeping table so it never runs again.

> There is no migration rollback (no `down` command): the utility only creates and applies migrations. This is intentional — design migrations so they can be safely layered on top of previous ones, without relying on a rollback.

## Creating migrations

To create a new migration, run the `create` command:

```bash
./exec/migrate create add_users_table ../config.json ../app/migrations/s1
```

| Argument               | Description                                          |
| ---------------------- | ---------------------------------------------------- |
| `add_users_table`      | migration name (used in the file name)               |
| `../config.json`       | path to the configuration file                       |
| `../app/migrations/s1` | target directory where the migration file is created |

A file named `YYYY-MM-DD_HH-MM-SS_add_users_table.c` appears in the target directory, pre-filled with a template:

```c
#include <stdlib.h>

#include "dbquery.h"
#include "dbresult.h"

int up(const char* dbid) {
    dbresult_t* result = dbquery(dbid, "", NULL);

    int res = dbresult_ok(result);

    dbresult_free(result);

    return res;
}
```

Fill the body of `up()` with the SQL statements that change the database structure. Query parameters are passed as the third argument to `dbquery` — an `array_t*` (or `NULL` when there are none). Example migration creating a `news` table:

```c
#include <stdlib.h>

#include "dbquery.h"
#include "dbresult.h"

int up(const char* dbid) {
    dbresult_t* result = dbquery(dbid,
        "CREATE TABLE news"
        "("
            "id    bigserial     NOT NULL PRIMARY KEY, "
            "name  varchar(100)  NOT NULL DEFAULT '', "
            "text  text          NOT NULL DEFAULT '' "
        ")"
    , NULL);

    int res = dbresult_ok(result);

    dbresult_free(result);

    return res;
}
```

::: tip Table names and schemas
In the examples above, `news` is created in the default schema. To name the schema explicitly (as the project's own migrations do — `public.user`, `public.permission`), just qualify the table: `"CREATE TABLE public.news ..."`.
:::

## Applying migrations

To bring the database up to date, use the `up` command. The number of migrations is set by an optional argument — either a number or the `all` keyword:

```bash
# apply all pending migrations
./exec/migrate up all ../config.json postgresql.p1 s1

# apply the next 2 migrations
./exec/migrate up 2 ../config.json postgresql.p1 s1

# no count given — applies exactly 1 migration
./exec/migrate up ../config.json postgresql.p1 s1
```

| Argument          | Description                                                                  |
| ----------------- | ---------------------------------------------------------------------------- |
| `up`              | apply-migrations command                                                     |
| `all` \| `N`      | how many to apply (`all` — every pending one; a number — exactly `N`)        |
| `../config.json`  | path to the configuration file                                               |
| `postgresql.p1`   | database identifier `<driver>.<host_id>` from the `databases` section        |
| `s1`              | migration set identifier (subdirectory)                                      |

> If no count is given, exactly one migration is applied.

The utility scans the `./migrations/<server_id>/` directory (relative to the current working directory), loads each `.so` via `dlopen`, and for migrations not yet present in the `migration` table calls their `up()` function. Each successfully completed migration is recorded as a new row in `migration`, letting the tool tell applied and pending migrations apart.

::: tip Execution order
Files are sorted by name in ascending order, so migrations are applied strictly in the order they were created (by the timestamp embedded in the file name).
:::

## Identifiers

### Database (`dbid`)

The format is `<driver>.<host_id>`, where `host_id` comes from `config.json`:

| dbid             | Driver      | Entry in `config.json`               |
| ---------------- | ----------- | ------------------------------------ |
| `postgresql.p1`  | PostgreSQL  | `databases.postgresql[].host_id = "p1"` |
| `mysql.m1`       | MySQL       | `databases.mysql[].host_id = "m1"`      |
| `redis.r1`       | Redis       | `databases.redis[].host_id = "r1"`      |
| `sqlite.db1`     | SQLite      | `databases.sqlite[].host_id = "db1"`    |

A migration is applied only to the database whose `dbid` you pass to `up`.

### Migration set (`server_id`)

`server_id` (`s1`, `s2`, …) is a subdirectory holding an independent set of migrations (each compiled separately — see `app/migrations/<server_id>/`). It lets you maintain several independent migration chains within a single project.

## The `migration` table

On the first `up` run, the utility automatically creates a `migration` bookkeeping table in the target database. It contains at least:

| Column         | Type     | Purpose                                          |
| -------------- | -------- | ------------------------------------------------ |
| `version`      | text     | migration file name (unique version identifier)  |
| `apply_time`   | bigint   | Unix timestamp of when it was applied            |

Before executing a migration, the utility checks whether its `version` is already in this table — applied migrations are skipped, making `up all` idempotent and safe to call repeatedly.
