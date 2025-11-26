---
outline: deep
description: Complete C Web Framework configuration description. Server, database, middleware, rate limiting, storage, session and email settings.
---

# Configuration file

Application configuration is stored in `config.json`. The file contains settings for servers, databases, storage, sessions, email, and other components.

## Section main

Main application settings.

### workers <Badge type="info" text="number"/>

Number of worker threads for handling connections and reading/writing data.

### threads <Badge type="info" text="number"/>

Number of threads for processing requests and generating responses.

### reload <Badge type="info" text="soft | hard"/>

Hot reload mode:

* `soft` — reload while keeping active connections
* `hard` — reload with forced connection closing

### buffer_size <Badge type="info" text="number"/>

Buffer size for socket read/write operations (in bytes).

### client_max_body_size <Badge type="info" text="number"/>

Maximum request body size (in bytes).

### tmp <Badge type="info" text="string"/>

Path to temporary files directory.

### gzip <Badge type="info" text="string array"/>

List of MIME types for automatic compression. Leave empty if compression is not required.

### log <Badge type="info" text="object"/>

Logging system settings:

```json
"log": {
    "enabled": true,
    "level": "info"
}
```

#### enabled <Badge type="info" text="boolean"/>

Enables or disables logging. When `false`, all logging functions will be ignored.

#### level <Badge type="info" text="string"/>

Minimum logging level. Messages with lower priority will be filtered out.

Valid values (from most critical to least critical):
- `emerg` — system is unusable (priority 0)
- `alert` — action must be taken immediately (priority 1)
- `crit` — critical conditions (priority 2)
- `err` or `error` — error conditions (priority 3)
- `warning` or `warn` — warning conditions (priority 4)
- `notice` — normal but significant condition (priority 5)
- `info` — informational messages (priority 6)
- `debug` — debug-level messages (priority 7)

::: tip Recommendations
- **Production:** use `info` or `notice` for balance between useful information and performance
- **Development:** use `debug` for maximum detailed logging
- **Critical systems:** use `warning` or `error` for minimal logging of important events only
:::

## Section migrations

### source_directory <Badge type="info" text="string"/>

Path to directory with migration files.

## Section servers

HTTP/WebSocket server configuration. Each server has a unique identifier (e.g., `s1`, `s2`).

### domains <Badge type="info" text="string array"/>

List of domains bound to the server. Supported formats:

* Exact names: `example.com`
* Wildcards: `*.example.com`
* Regular expressions: `(api|www).example.com`

### ip <Badge type="info" text="string"/>

IP address to listen on, e.g., `127.0.0.1`.

### port <Badge type="info" text="number"/>

Server port (typically `80` for HTTP, `443` for HTTPS).

### root <Badge type="info" text="string"/>

Path to static files root directory.

### ratelimits <Badge type="info" text="object"/>

Rate limiting settings:

```json
"ratelimits": {
    "default": { "burst": 15, "rate": 15 },
    "strict": { "burst": 1, "rate": 0 }
}
```

* `burst` — maximum number of requests in peak
* `rate` — token replenishment rate

### http <Badge type="info" text="object"/>

HTTP routes and middleware configuration.

#### ratelimit <Badge type="info" text="string"/>

Default rate limiting profile name for all routes.

#### middlewares <Badge type="info" text="string array"/>

List of middleware applied to all routes:

```json
"middlewares": ["middleware_auth", "middleware_log"]
```

#### routes <Badge type="info" text="object"/>

HTTP request routes:

```json
"routes": {
    "/api/users": {
        "GET": {
            "file": "/path/to/handler.so",
            "function": "get_users",
            "ratelimit": "strict"
        },
        "POST": {
            "file": "/path/to/handler.so",
            "function": "create_user"
        }
    },
    "/api/users/{id|\\d+}": {
        "PATCH": {
            "file": "/path/to/handler.so",
            "function": "update_user"
        }
    }
}
```

Route parameters:
* `file` — path to .so handler file
* `function` — handler function name
* `ratelimit` — override rate limiting profile for route

#### redirects <Badge type="info" text="object"/>

Redirect rules with regular expression support:

```json
"redirects": {
    "/old-path": "/new-path",
    "/number/(\\d)/(\\d)": "/digit/{1}/{2}"
}
```

Capture groups are available via `{1}`, `{2}`, etc.

### websockets <Badge type="info" text="object"/>

WebSocket configuration. Structure is similar to `http` section:

```json
"websockets": {
    "default": {
        "file": "/path/to/ws_handler.so",
        "function": "default_handler"
    },
    "routes": {
        "/ws": {
            "GET": { "file": "/path/to/ws_handler.so", "function": "connect" },
            "POST": { "file": "/path/to/ws_handler.so", "function": "message" }
        }
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

## Section databases

Global database configuration. Multiple connections to each database type are supported.

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

## Section storages

File storage configuration.

### Local storage

```json
"storages": {
    "local": {
        "type": "filesystem",
        "root": "/var/www/storage"
    }
}
```

### S3-compatible storage

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

## Section sessions

User session configuration.

```json
"sessions": {
    "driver": "storage",
    "host_id": "redis.r1",
    "storage_name": "sessions",
    "lifetime": 3600
}
```

* `driver` — session storage driver (`storage` or `file`)
* `host_id` — storage identifier for sessions
* `lifetime` — session lifetime in seconds

## Section mail

Email sending configuration.

```json
"mail": {
    "dkim_private": "/path/to/dkim_private.pem",
    "dkim_selector": "mail",
    "host": "example.com"
}
```

## Section mimetypes

MIME type to file extension mapping:

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
        "buffer_size": 16384,
        "client_max_body_size": 110485760,
        "tmp": "/tmp",
        "gzip": ["text/html", "text/css", "application/json", "application/javascript"],
        "log": {
            "enabled": true,
            "level": "info"
        }
    },
    "migrations": {
        "source_directory": "/path/to/migrations"
    },
    "servers": {
        "s1": {
            "domains": ["example.com", "*.example.com"],
            "ip": "127.0.0.1",
            "port": 443,
            "root": "/var/www/html",
            "ratelimits": {
                "default": { "burst": 15, "rate": 15 },
                "strict": { "burst": 1, "rate": 0 }
            },
            "http": {
                "ratelimit": "default",
                "middlewares": ["middleware_auth"],
                "routes": {
                    "/api/users": {
                        "GET": { "file": "handlers/libapi.so", "function": "get_users" },
                        "POST": { "file": "handlers/libapi.so", "function": "create_user" }
                    }
                },
                "redirects": {
                    "/old": "/new"
                }
            },
            "websockets": {
                "routes": {
                    "/ws": {
                        "GET": { "file": "handlers/libws.so", "function": "connect" }
                    }
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
            "host_id": "p1",
            "ip": "127.0.0.1",
            "port": 5432,
            "dbname": "mydb",
            "user": "dbuser",
            "password": "dbpass",
            "connection_timeout": 3
        }],
        "redis": [{
            "host_id": "r1",
            "ip": "127.0.0.1",
            "port": 6379,
            "dbindex": 0,
            "user": "",
            "password": ""
        }]
    },
    "storages": {
        "files": {
            "type": "filesystem",
            "root": "/var/www/storage"
        }
    },
    "sessions": {
        "driver": "storage",
        "host_id": "redis.r1",
        "storage_name": "sessions",
        "lifetime": 3600
    },
    "mail": {
        "dkim_private": "/path/to/dkim_private.pem",
        "dkim_selector": "mail",
        "host": "example.com"
    },
    "mimetypes": {
        "text/html": ["html", "htm"],
        "text/css": ["css"],
        "application/json": ["json"],
        "application/javascript": ["js"],
        "image/png": ["png"],
        "image/jpeg": ["jpeg", "jpg"]
    }
}
```
