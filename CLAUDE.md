# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

A high-performance event-driven C web framework (`cpdy`) for Linux. The repository is a monorepo: `backend/` contains the C framework core (as a git submodule) and an example application; `frontend/` contains a VitePress documentation site (English, `frontend/docs/en/`).

## Build Commands

```bash
# Build (from backend/)
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DINCLUDE_POSTGRESQL=yes \
         -DINCLUDE_MYSQL=yes \
         -DINCLUDE_REDIS=yes \
         -DINCLUDE_SQLITE=yes
cmake --build . -j$(nproc)

# Debug build (enables AddressSanitizer + leak detection + -fanalyzer)
cmake .. -DCMAKE_BUILD_TYPE=Debug

# RelWithDebInfo — optimized with debug info (also enables sanitizers here)
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo

# Build with unit tests (core/tests)
cmake .. -DBUILD_TESTS=yes
cmake --build . -j$(nproc)

# Run server (CLI: cpdy -c <config>)
./exec/cpdy -c ../config.json

# Run migrations — positional args, NO -c flag, NO down/rollback command:
#   create <name> <config> <target_dir>
#   up [number|all] <config> <db_host> <server_id>
./exec/migrate create add_users_table ../config.json ../app/migrations/s1
./exec/migrate up     ../config.json postgresql.p1 s1   # apply pending
./exec/migrate up all ../config.json postgresql.p1 s1   # apply all
./exec/migrate up 2   ../config.json postgresql.p1 s1   # apply next 2

# Frontend docs (from frontend/)
npm run docs:dev      # vitepress dev server
npm run docs:build    # static build
npm run docs:preview
```

`<db_host>` is the database id (`<driver>.<host_id>`, e.g. `postgresql.p1`, `mysql.m1`, `redis.r1`, `sqlite.<id>`); `<server_id>` is the migration folder (`s1`, `s2`, …).

## Architecture

### Core (`backend/core/` — git submodule)
- **Apps** (`core/apps/`): the two executables — `server/main.c` (→ `cpdy`) and `migrate/main.c` (→ `migrate`).
- **Server runtime** (`core/src/`): epoll event loop + worker processes/threads (`server/`, `multiplexing/`, `thread/`, `connection/`, `socket/`, `signal/`), routing (`route/`), virtual hosts/domains incl. regex + IDN (`domain/`), config loading (`config/`), MIME types (`mimetype/`), dynamic `.so` loading (`moduleloader/`), rate limiting (`ratelimiter/`), OpenSSL helpers (`openssl/`), broadcasting (`broadcast/`).
- **Protocols** (`core/protocols/`): HTTP/1.1 server + client (`http/`), WebSocket (`websocket/`), SMTP client with DKIM (`smtp/`).
- **Framework** (`core/framework/`): ORM model system (`model/`), DB layer (`database/` — PostgreSQL/MySQL/Redis/SQLite), sessions (`session/`), storage FS/S3 (`storage/`), template engine (`view/`), middleware (`middleware/`), task scheduler (`taskmanager/`), i18n (`translation/`).
- **Utilities** (`core/misc/`): dynamic strings w/ SSO (`str.h`), arrays (`array.h`), hashmaps (`hashmap.h`/`map.h`), ordered queue (`cqueue.h`), buffer objects (`bufo.h`), JSON parser/generator (`json.h`), JWT (`jwt.h`), UUID (`uuid.h`), base64 (`base64.h`), SHA-1/256 (`sha1.h`/`sha256.h`), random (`random.h`), UTF-8 (`utf8.h`), i18n + IDN utils (`i18n.h`/`idn_utils.h`), query parser (`queryparser.h`), logging (`log.h`), gzip (`gzip.h`), files (`file.h`).

### Application (`backend/app/`)
- **Routes** (`app/routes/`): HTTP/WebSocket handlers compiled as `.so` shared libraries, dynamically loaded at runtime. Each handler receives `httpctx_t*` or `wsctx_t*`. Example route groups: `auth/`, `index/`, `ws/`, `files/`, `models/`, `email/`, `db/`, `json/`, `httpclient/`, `middleware/`.
- **Models** (`app/models/`): ORM model definitions using `mfield_*` macros (e.g. `mfield_int`, `mfield_text`). CRUD wrappers `*_create`/`*_get`/`*_update`/`*_delete` (e.g. `user_create`) delegate to the generic `model_create(dbid, arg)` etc. `*view` models back JOIN queries; `prepare_statements.c` registers named prepared statements.
- **Contexts** (`app/contexts/`): `httpctx.c`/`wsctx.c` — request/response helpers bound onto `httpctx_t`/`wsctx_t`.
- **Middleware** (`app/middlewares/`): functions returning `int` (0=reject, 1=allow), invoked with the `middleware()` macro; `middlewarelist.c` registers them by name for `config.json`.
- **Auth** (`app/auth/`): PBKDF2-HMAC-SHA256 password hashing (140000 iterations, 16-byte salt, 32-byte hash), email/password validators, `authenticate()` / `authenticate_by_cookie()`.
- **Migrations** (`app/migrations/s1/`, `s2/`): timestamped C files implementing `int up(const char* dbid)`, compiled into the `migrate` binary.
- **Broadcasting** (`app/broadcasting/`): named channel definitions for WebSocket fan-out.
- **Views** (`app/views/`): template files (`.tpl`).

### Configuration
All runtime config is in `backend/config.json` (no .env files). Sections: `main` (workers, threads, buffer sizes, `log`, `env`), `migrations` (source dir), `translations` (i18n locale paths), `task_manager` (scheduled `.so` tasks), `servers` (virtual hosts: domains w/ regex, redirects w/ capture-group substitution `{1}`, routes, websockets, TLS, ratelimits), `databases` (PostgreSQL/MySQL/Redis/SQLite pools, referenced as `<driver>.<host_id>`), `storages` (filesystem/S3), `sessions` (named map; drivers `filesystem`/`redis`/`database`, each with a `secret`), `mail` (DKIM), `mimetypes`.

### Response Patterns (`httpresponse.h`)
- `send_data()` / `send_datan()` — text/HTML (optionally with length)
- `send_json()` — serialize a `json_doc_t*`
- `send_model()` / `send_models()` — JSON from one ORM object / an array of them
- `send_file()` / `send_filen()` / `send_filef()` — file download (formatted path can read from a named storage)
- `send_default()` — HTTP status code only

### Database Patterns (`dbquery.h`)
- `dbquery(dbid, sql, params)` — parameterized query with an explicit `array_t*` of params
- `dbqueryf(dbid, sql[, params])` — variadic form, convenient for inline SQL (used in migrations; pass `NULL` for no params)
- `dbinsert` / `dbupdate` / `dbdelete` / `dbselect` / `dbprepared` — table-level and named-statement helpers
- Parameter syntax: `:name` (prepared statement) or `@name` (immediate substitution); build params with `mparams_*` macros
- Iterate results via `dbresult_*` (`dbresult_ok`, `dbresult_query_rows`, `dbresult_cell`, …); always `dbresult_free()`
- Transactions: `dbbegin` / `dbcommit` / `dbrollback` (with isolation level)
- All queries must use prepared statements for SQL injection protection

### Session Patterns (`framework/session/`)
- Three drivers, selected per named session in `config.json`: `sessionfile` (filesystem), `sessionredis` (Redis), `sessiondb` (database)
- Session secrets are protected with **AES-256-GCM** (`aes256gcm.h`) — set the `secret` field per session

### Memory Management
- Every allocated resource has a corresponding `*_free()` function — always call it
- Use `explicit_bzero()` for sensitive data before freeing
