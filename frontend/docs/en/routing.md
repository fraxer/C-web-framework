---
outline: deep
description: C Web Framework routing system. HTTP and WebSocket route configuration, dynamic parameters, middleware, rate limiting and redirects.
---

# Routing

Routing defines the mapping between URLs and handlers. Both HTTP and WebSocket protocols are supported. Routes are described in the `servers.<id>` section of `config.json` — inside the `http` and `websockets` objects.

## Basic configuration

The key of the `routes` object is a path; the value is a "method → handler" object:

```json
"http": {
    "routes": {
        "/": {
            "GET": { "file": "handlers/libindex.so", "function": "index" }
        },
        "/api/users": {
            "GET":  { "file": "handlers/libapi.so", "function": "get_users" },
            "POST": { "file": "handlers/libapi.so", "function": "create_user" }
        }
    }
}
```

Handler fields:

| Field         | Type   | Description                                                                          |
|---------------|--------|--------------------------------------------------------------------------------------|
| `file`        | string | Path to the handler `.so` (relative to the process root, or absolute)               |
| `function`    | string | Name of the handler function                                                         |
| `static_file` | string | Path to a static file; when set, the file is served instead of calling `file`/`function` |
| `ratelimit`   | string | Name of the rate limiting profile for this route (overrides `http.ratelimit`)        |

## Route matching

Routes are checked **in declaration order** — the first match wins. There are two kinds of routes:

* **Primitive** — a path with no parameters and no regex characters (`/`, `/api/users`). Matched by a fast character-by-character comparison.
* **Parameterized / regex** — a path with `{...}` parameters or regex characters (`*`, `[`, `(`, `+`, `^`, `$`, `|`). Compiled into a PCRE regular expression.

If the matched route has no handler for the current method, matching continues with the remaining routes. If no route matches, a `404` is returned.

## Dynamic parameters

Route parameters are specified in the format `{name|pattern}`, where `pattern` is a PCRE regular expression:

```json
"routes": {
    "/api/users/{id|\\d+}": {
        "GET":    { "file": "handlers/libapi.so", "function": "get_user" },
        "PUT":    { "file": "handlers/libapi.so", "function": "update_user" },
        "DELETE": { "file": "handlers/libapi.so", "function": "delete_user" }
    },
    "/api/posts/{slug|[a-z0-9-]+}": {
        "GET": { "file": "handlers/libapi.so", "function": "get_post" }
    }
}
```

Parameter values are added to the query string and read as query parameters.

### Extracting parameters in a handler

```c
#include "query.h"

void get_user(httpctx_t* ctx) {
    int ok = 0;
    const char* id = query_param_char(ctx->request->query_, "id", &ok);
    // id = "123" for request /api/users/123

    if (!ok) {
        // handle error
        return;
    }

    // Convert to number
    int user_id = atoi(id);

    // Find user...
}

void get_post(httpctx_t* ctx) {
    int ok = 0;
    const char* slug = query_param_char(ctx->request->query_, "slug", &ok);
    // slug = "my-first-post" for /api/posts/my-first-post
}
```

::: warning Named parameters and regular expressions
Named parameters `{name|pattern}` cannot be mixed with "raw" regular-expression characters (`* [ ] ( ) + ^ | $`) in the same path — the parameter itself is the regular expression. For free-form patterns, use full regular expressions (see below).
:::

## Regular expressions

For complex patterns, use full regular expressions. Capture groups are available in the handler as numbered parameters:

```json
"routes": {
    "^/managers/{name|[a-z]+}$": {
        "GET": { "file": "handlers/libapi.so", "function": "get_manager" }
    },
    "^/files/(.+)\\.pdf$": {
        "GET": { "file": "handlers/libfiles.so", "function": "get_pdf" }
    }
}
```

## HTTP methods

Supported methods:

| Method     | Description                  |
|------------|------------------------------|
| `GET`      | Retrieve a resource          |
| `POST`     | Create a resource            |
| `PUT`      | Full update                  |
| `PATCH`    | Partial update               |
| `DELETE`   | Delete a resource            |
| `HEAD`     | Headers without body         |
| `OPTIONS`  | CORS preflight requests      |

```json
"/api/resource": {
    "GET":     { "file": "handlers/lib.so", "function": "get" },
    "POST":    { "file": "handlers/lib.so", "function": "create" },
    "PUT":     { "file": "handlers/lib.so", "function": "replace" },
    "PATCH":   { "file": "handlers/lib.so", "function": "update" },
    "DELETE":  { "file": "handlers/lib.so", "function": "delete" },
    "HEAD":    { "file": "handlers/lib.so", "function": "head" },
    "OPTIONS": { "file": "handlers/lib.so", "function": "options" }
}
```

## Static files

For routes that should serve static files without processing, use the `static_file` parameter instead of `file` and `function`:

```json
"routes": {
    "/": {
        "GET": { "static_file": "public/index.html" }
    },
    "/favicon.ico": {
        "GET": { "static_file": "public/favicon.ico" }
    },
    "/robots.txt": {
        "GET": { "static_file": "public/robots.txt" }
    }
}
```

The file path is relative to the server's `root` directory.

### Combining with handlers

Static files and handlers can be combined in the same route for different methods:

```json
"/api/docs": {
    "GET":  { "static_file": "public/api-docs.html" },
    "POST": { "file": "handlers/libapi.so", "function": "update_docs" }
}
```

### Rate limiting for static files

Rate limiting can be applied to static files:

```json
"/downloads/report.pdf": {
    "GET": {
        "static_file": "files/report.pdf",
        "ratelimit": "downloads"
    }
}
```

::: tip When to use static_file
Use `static_file` for files that don't require processing: HTML pages, images, documents, favicon, etc. This is more efficient than creating a handler for each file.
:::

## Middleware

Middleware are executed before the handler and apply to all routes of the section (`http.middlewares` / `websockets.middlewares`):

### Global middleware

```json
"http": {
    "middlewares": ["middleware_auth", "middleware_log"],
    "routes": {
        // All routes pass through middleware_auth and middleware_log
    }
}
```

### Middleware in a handler

```c
#include "httpmiddlewares.h"

void protected_handler(httpctx_t* ctx) {
    // Authentication check
    middleware(
        middleware_http_auth(ctx)
    )

    // This code will only execute if the user is authorized
    user_t* user = httpctx_get_user(ctx);
    ctx->response->send_data(ctx->response, "Welcome!");
}
```

## Rate Limiting

Rate limiting profiles are defined at the server level — in the `ratelimits` section (alongside `http` and `websockets`). Each profile defines `burst` (bucket capacity / peak number of requests) and `rate` (token refill rate per second):

```json
"s1": {
    "ratelimits": {
        "default": { "burst": 15,  "rate": 15 },
        "strict":  { "burst": 1,   "rate": 0 },
        "api":     { "burst": 100, "rate": 100 }
    },
    "http": {
        "ratelimit": "default",
        "routes": {
            "/api/users": {
                "GET":  { "file": "handlers/libapi.so", "function": "get_users",   "ratelimit": "api" },
                "POST": { "file": "handlers/libapi.so", "function": "create_user", "ratelimit": "strict" }
            }
        }
    }
}
```

* `http.ratelimit` — the default profile for all HTTP routes of the server.
* A `ratelimit` on a specific route overrides the profile for that route.

## Redirects

Redirects are described in the `http.redirects` section. The key is a PCRE regular expression; the value is the target path. Capture groups are substituted via `{1}`, `{2}`, etc.:

```json
"http": {
    "redirects": {
        "/user": "/persons",
        "/user(.*)/(\\d)": "/user-{1}-{2}",
        "/section1/(\\d+)/section2/(\\d+)/section3": "/one/{1}/two/{2}/three"
    }
}
```

## WebSocket routes

WebSocket is configured in the `websockets` section. It supports a default handler (`default`), a shared `ratelimit`, `middlewares` and `routes`:

```json
"websockets": {
    "default": {
        "file": "handlers/libws.so",
        "function": "ws_default"
    },
    "ratelimit": "default",
    "routes": {
        "/ws": {
            "GET":  { "file": "handlers/libws.so", "function": "ws_connect" },
            "POST": { "file": "handlers/libws.so", "function": "ws_message" }
        },
        "/ws/chat/{room|\\d+}": {
            "GET":  { "file": "handlers/libws.so", "function": "ws_join_room" },
            "POST": { "file": "handlers/libws.so", "function": "ws_send_message" }
        }
    }
}
```

* `default` — the handler called when no route matches. If the `websockets` section is not set, a built-in default handler is used.
* `ratelimit` — the default rate limiting profile for all WS routes.
* `middlewares` — middleware applied to all WS routes.

Supported methods for WebSocket routes: `GET`, `POST`, `PATCH`, `DELETE`.

### WebSocket handler

```c
#include "websockets.h"

void ws_connect(wsctx_t* ctx) {
    ctx->response->send_text(ctx->response, "Connected");
}

void ws_message(wsctx_t* ctx) {
    char* message = websocketsrequest_payload(ctx->request->protocol);

    if (message) {
        ctx->response->send_text(ctx->response, message);
        free(message);
    }
}
```

## Configuration examples

### REST API

```json
"http": {
    "middlewares": ["middleware_cors"],
    "ratelimit": "default",
    "routes": {
        "/api/v1/users": {
            "GET":  { "file": "handlers/libusers.so", "function": "list" },
            "POST": { "file": "handlers/libusers.so", "function": "create" }
        },
        "/api/v1/users/{id|\\d+}": {
            "GET":    { "file": "handlers/libusers.so", "function": "show" },
            "PUT":    { "file": "handlers/libusers.so", "function": "update" },
            "DELETE": { "file": "handlers/libusers.so", "function": "delete" }
        },
        "/api/v1/auth/login": {
            "POST": { "file": "handlers/libauth.so", "function": "login" }
        },
        "/api/v1/auth/logout": {
            "POST": { "file": "handlers/libauth.so", "function": "logout" }
        }
    }
}
```

### Mixed site (pages + API)

```json
"http": {
    "routes": {
        "/": {
            "GET": { "file": "handlers/libpages.so", "function": "home" }
        },
        "/about": {
            "GET": { "file": "handlers/libpages.so", "function": "about" }
        },
        "/api/data": {
            "GET": { "file": "handlers/libapi.so", "function": "get_data" }
        }
    }
}
```
