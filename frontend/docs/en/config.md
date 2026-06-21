---
outline: deep
description: Complete configuration reference for C Web Framework. Servers, databases, middleware, rate limiting, storages, sessions, tasks and email.
---

# Configuration file

Application configuration lives in `config.json` (passed via `cpdy -c <config>`). It describes servers, databases, storages, sessions, the task scheduler, email, and other components. Reload the configuration without stopping the server with `SIGUSR1` (`pkill -USR1 cpdy`) — the reload behavior is controlled by `main.reload`.

## main section

Core application settings.

### workers <Badge type="info" text="number"/>

Number of worker processes/threads that accept connections and read/write data.

### threads <Badge type="info" text="number"/>

Number of threads that process requests and build responses.

### reload <Badge type="info" text="soft | hard"/>

Hot-reload mode (triggered by `SIGUSR1`):

* `soft` (default) — reload while keeping active connections
* `hard` — reload and forcibly close active connections

### client_max_body_size <Badge type="info" text="number"/>

Maximum request body size in bytes.

### tmp <Badge type="info" text="string"/>

Path to the temporary files directory.

### gzip <Badge type="info" text="array of strings"/>

List of MIME types to compress automatically. Leave empty to disable compression.

### log <Badge type="info" text="object"/>

Logging settings:

```json
"log": {
    "enabled": true,
    "level": "info"
}
```

#### enabled <Badge type="info" text="boolean"/>

Enables or disables logging. When `false`, all logging functions are ignored.

#### level <Badge type="info" text="string"/>

Minimum log level. Lower-priority messages are filtered out. Allowed values (most severe first):

- `emerg` — system is unusable (0)
- `alert` — action must be taken immediately (1)
- `crit` — critical condition (2)
- `err` / `error` — errors (3)
- `warning` / `warn` — warnings (4)
- `notice` — normal but significant messages (5)
- `info` — informational messages (6)
- `debug` — debug-level messages (7)

::: tip Recommendations
- **Production:** `info` or `notice` — a balance between detail and performance.
- **Development:** `debug` — most verbose logging.
- **Critical systems:** `warning`/`error` — only significant events.
:::

### env <Badge type="info" text="object"/>

A free-form user key-value store. Values (`string`/`number`/`bool`/`null`) are copied as-is and accessed in code through `env_get_*` functions:

```json
"env": {
    "refresh_token_expiration": 15552000,
    "feature_x_enabled": true,
    "app_name": "backend"
}
```

```c
long long ttl = env_get_llong("refresh_token_expiration", 3600);
int enabled  = env_get_bool("feature_x_enabled", 0);
const char* name = env_get_string("app_name", "default");
```

Available: `env_get_string`, `env_get_int`, `env_get_llong`, `env_get_bool`, `env_get_double`, `env_get_ldouble` — each takes a key and a default value.

## migrations section

### source_directory <Badge type="info" text="string"/>

Path to the directory containing migration sources (used by the `migrate` utility).

## translations section

List of localization (i18n) domains. Each entry specifies a text domain and the path to its `.mo` catalog directory:

```json
"translations": [
    { "domain": "backend", "path": "/app/locale" }
]
```

* `domain` — text domain name (required)
* `path` — path to the translations catalog (required)

## task_manager section

List of background tasks loaded from `.so` files and run by the scheduler. Each task is an object with the common fields `name`, `type`, `file`, `function` plus schedule fields depending on `type`.

| `type` | Schedule fields | Description |
|--------|-----------------|-------------|
| `interval` | `interval` | Run every N seconds (≥ 1) |
| `daily` | `hour`, `minute` | Daily at `hh:mm` |
| `weekly` | `weekday`, `hour`, `minute` | Weekly on the given weekday |
| `monthly` | `day`, `hour`, `minute` | Monthly on the given day |

`weekday` accepts `sunday`…`saturday`; `hour` is 0–23, `minute` is 0–59, `day` is 1–31.

```json
"task_manager": [
    {
        "name": "cleanup_expired_tokens",
        "type": "interval",
        "interval": 60,
        "file": "/app/build/exec/handlers/tasks/libtasks.so",
        "function": "cleanup_authorization_codes"
    },
    {
        "name": "nightly_report",
        "type": "daily",
        "hour": 3,
        "minute": 30,
        "file": "/app/build/exec/handlers/tasks/libtasks.so",
        "function": "send_report"
    }
]
```

## servers section

HTTP/WebSocket server configuration. Each server is a named entry (`s1`, `s2`, …).

### domains <Badge type="info" text="array of strings"/>

Domains bound to the server. Supported:

* Exact names: `example.com`
* Wildcards: `*.example.com`, `mail.*`
* Regular expressions: `(api|www).example.com`, `(.1|.*|a3).example.com`

### ip <Badge type="info" text="string"/>

Listen IP address, e.g. `127.0.0.1`.

### port <Badge type="info" text="number"/>

Server port (typically `80` for HTTP, `443` for HTTPS).

### root <Badge type="info" text="string"/>

Path to the static files root directory.

### index <Badge type="info" text="string"/>

Index file name returned for directories (e.g. `index.html`).

### ratelimits <Badge type="info" text="object"/>

Named rate-limiting profiles. Each profile defines `burst` (peak number of requests) and `rate` (token refill rate):

```json
"ratelimits": {
    "one":   { "burst": 1,  "rate": 0  },
    "two":   { "burst": 15, "rate": 15 }
}
```

### http <Badge type="info" text="object"/>

HTTP routes, middleware, and redirects.

#### ratelimit <Badge type="info" text="string"/>

Default rate-limiting profile for all routes in this section.

#### middlewares <Badge type="info" text="array of strings"/>

Middlewares applied to all routes (names from the middleware registry):

```json
"middlewares": ["middleware_http_auth"]
```

#### routes <Badge type="info" text="object"/>

HTTP routes. The key is the path (supporting regular expressions and named parameters); the value is a `METHOD → { handler }` object:

```json
"routes": {
    "/api/users": {
        "GET":  { "file": "handlers/models/lib_modeluser.so", "function": "list", "ratelimit": "two" },
        "POST": { "file": "handlers/models/lib_modeluser.so", "function": "create" }
    },
    "/api/users/{id|\\d+}": {
        "PATCH": { "file": "handlers/models/lib_modeluser.so", "function": "update" }
    }
}
```

Handler options:

* `file` — path to the handler `.so`
* `function` — handler function name
* `static_file` — path to a static file; when set, the request serves the file instead of invoking `file`/`function`
* `ratelimit` — override the rate-limiting profile for this route

#### redirects <Badge type="info" text="object"/>

Redirect rules with regular-expression support. Capture groups are substituted via `{1}`, `{2}`, …:

```json
"redirects": {
    "/user": "/persons",
    "/user(.*)/(\\d)": "/user-{1}-{2}",
    "/section1/(\\d+)/section2/(\\d+)/section3": "/one/{1}/two/{2}/three"
}
```

### websockets <Badge type="info" text="object"/>

WebSocket configuration. Supports a default handler (`default`), a shared `ratelimit`, `middlewares`, and `routes`:

```json
"websockets": {
    "default": { "file": "handlers/ws/lib_wsindex.so", "function": "default_" },
    "routes": {
        "/": { "GET": { "file": "handlers/ws/lib_wsindex.so", "function": "echo" } }
    }
}
```

### tls <Badge type="info" text="object"/>

TLS/SSL settings for HTTPS:

```json
"tls": {
    "fullchain": "/path/to/fullchain.pem",
    "private": "/path/to/privkey.pem",
    "ciphers": "TLS_AES_256_GCM_SHA384 TLS_CHACHA20_POLY1305_SHA256"
}
```

## databases section

Database connection configuration. Each driver is an array of hosts; connections are addressed in code as `<driver>.<host_id>` (e.g. `postgresql.p1`, `redis.r1`, `sqlite.local`). Only drivers enabled at build time (`-DINCLUDE_*`) are compiled.

### postgresql

```json
"postgresql": [{
    "host_id": "p1",
    "ip": "127.0.0.1",
    "port": 5432,
    "dbname": "mydb",
    "user": "dbuser",
    "password": "dbpass",
    "connection_timeout": 3
}]
```

### mysql

```json
"mysql": [{
    "host_id": "m1",
    "ip": "127.0.0.1",
    "port": 3306,
    "dbname": "mydb",
    "user": "dbuser",
    "password": "dbpass"
}]
```

### redis

```json
"redis": [{
    "host_id": "r1",
    "ip": "127.0.0.1",
    "port": 6379,
    "dbindex": 0,
    "user": "",
    "password": ""
}]
```

### sqlite

```json
"sqlite": [{
    "host_id": "local",
    "path": "/var/lib/app/data.sqlite",
    "journal_mode": "WAL",
    "busy_timeout": 5000
}]
```

* `path` — database file path (`":memory:"` for in-memory); required
* `journal_mode` — `PRAGMA journal_mode` (default `WAL`)
* `busy_timeout` — `PRAGMA busy_timeout` in milliseconds (default `5000`)

## storages section

Named file storages. The type is set by the `type` field.

### filesystem

```json
"storages": {
    "local": {
        "type": "filesystem",
        "root": "/var/www/storage"
    }
}
```

### s3

```json
"storages": {
    "remote": {
        "type": "s3",
        "access_id": "your_access_id",
        "access_secret": "your_access_secret",
        "protocol": "https",
        "host": "s3.amazonaws.com",
        "port": "443",
        "bucket": "my-bucket",
        "region": "us-east-1"
    }
}
```

`region` is optional (defaults to the provider default).

## sessions section

A named map of session configurations. Each key is the session name used in code (`session_create("backend", data, ttl)`). All sessions are encrypted with **AES-256-GCM**; the key is derived from the required `secret` field.

The driver is set by the `driver` field:

| `driver` | Storage field | Description |
|----------|---------------|-------------|
| `filesystem` | `storage_name` | Files in the named storage (a name from the `storages` section) |
| `redis` | `host_id` | Redis, addressed as `redis.<host_id>` |
| `database` | `host_id` | Database, addressed as `<driver>.<host_id>` (e.g. `postgresql.p1`) |

```json
"sessions": {
    "backend": {
        "driver": "filesystem",
        "storage_name": "local",
        "secret": "change-me"
    },
    "scheduler": {
        "driver": "redis",
        "host_id": "redis.r1",
        "secret": "change-me"
    },
    "doc-editor": {
        "driver": "database",
        "host_id": "postgresql.p1",
        "secret": "change-me"
    }
}
```

The session lifetime is not set in the configuration — it is passed at creation: `session_create(name, data, duration)`.

## mail section

Email sending configuration with DKIM signing:

```json
"mail": {
    "dkim_private": "/path/to/dkim_private.pem",
    "dkim_selector": "mail",
    "host": "example.com"
}
```

## mimetypes section

Mapping of MIME types to file extensions:

```json
"mimetypes": {
    "text/html": ["html", "htm"],
    "text/css": ["css"],
    "application/json": ["json"],
    "application/javascript": ["js"],
    "image/png": ["png"],
    "image/jpeg": ["jpeg", "jpg"]
}
```

## Full configuration example

```json
{
    "main": {
        "workers": 4,
        "threads": 2,
        "reload": "hard",
        "client_max_body_size": 110485760,
        "tmp": "/tmp",
        "gzip": ["text/html", "text/css", "application/json", "application/javascript"],
        "log": { "enabled": true, "level": "info" },
        "env": { "refresh_token_expiration": 15552000 }
    },
    "migrations": {
        "source_directory": "/app/app/migrations"
    },
    "translations": [
        { "domain": "backend", "path": "/app/locale" }
    ],
    "task_manager": [
        {
            "name": "cleanup_expired_tokens",
            "type": "interval",
            "interval": 60,
            "file": "/app/build/exec/handlers/tasks/libtasks.so",
            "function": "cleanup_authorization_codes"
        }
    ],
    "servers": {
        "s1": {
            "domains": ["example.com", "*.example.com"],
            "ip": "127.0.0.1",
            "port": 443,
            "root": "/var/www/html",
            "index": "index.html",
            "ratelimits": {
                "default": { "burst": 15, "rate": 15 },
                "strict":  { "burst": 1,  "rate": 0  }
            },
            "http": {
                "ratelimit": "default",
                "middlewares": ["middleware_http_auth"],
                "routes": {
                    "/api/users": {
                        "GET":  { "file": "handlers/models/lib_modeluser.so", "function": "list" },
                        "POST": { "file": "handlers/models/lib_modeluser.so", "function": "create" }
                    },
                    "/robots.txt": {
                        "GET": { "static_file": "/var/www/html/robots.txt" }
                    }
                },
                "redirects": {
                    "/old": "/new"
                }
            },
            "websockets": {
                "default": { "file": "handlers/ws/lib_wsindex.so", "function": "default_" },
                "routes": {
                    "/ws": { "GET": { "file": "handlers/ws/lib_wsindex.so", "function": "connect" } }
                }
            },
            "tls": {
                "fullchain": "/path/to/fullchain.pem",
                "private": "/path/to/privkey.pem",
                "ciphers": "TLS_AES_256_GCM_SHA384 TLS_CHACHA20_POLY1305_SHA256"
            }
        }
    },
    "databases": {
        "postgresql": [{
            "host_id": "p1", "ip": "127.0.0.1", "port": 5432,
            "dbname": "mydb", "user": "dbuser", "password": "dbpass",
            "connection_timeout": 3
        }],
        "redis": [{
            "host_id": "r1", "ip": "127.0.0.1", "port": 6379,
            "dbindex": 0, "user": "", "password": ""
        }],
        "sqlite": [{
            "host_id": "local", "path": "/var/lib/app/data.sqlite"
        }]
    },
    "storages": {
        "local": { "type": "filesystem", "root": "/var/www/storage" }
    },
    "sessions": {
        "backend": {
            "driver": "filesystem",
            "storage_name": "local",
            "secret": "change-me"
        }
    },
    "mail": {
        "dkim_private": "/path/to/dkim_private.pem",
        "dkim_selector": "mail",
        "host": "example.com"
    },
    "mimetypes": {
        "text/html": ["html", "htm"],
        "application/json": ["json"],
        "application/javascript": ["js"],
        "image/png": ["png"]
    }
}
```
