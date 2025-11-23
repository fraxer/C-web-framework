---
outline: deep
description: C Web Framework routing system. HTTP and WebSocket route configuration, dynamic parameters, middleware and rate limiting.
---

# Routing

Routing defines the mapping between URLs and handlers. HTTP and WebSocket protocols are supported.

## Basic configuration

```json
"http": {
    "routes": {
        "/": {
            "GET": { "file": "handlers/libindex.so", "function": "index" }
        },
        "/api/users": {
            "GET": { "file": "handlers/libapi.so", "function": "get_users" },
            "POST": { "file": "handlers/libapi.so", "function": "create_user" }
        }
    }
}
```

## Dynamic parameters

Route parameters are specified in the format `{name|pattern}`:

```json
"routes": {
    "/api/users/{id|\\d+}": {
        "GET": { "file": "handlers/libapi.so", "function": "get_user" },
        "PUT": { "file": "handlers/libapi.so", "function": "update_user" },
        "DELETE": { "file": "handlers/libapi.so", "function": "delete_user" }
    },
    "/api/posts/{slug|[a-z0-9-]+}": {
        "GET": { "file": "handlers/libapi.so", "function": "get_post" }
    }
}
```

### Extracting parameters in a handler

```c
#include "query.h"

void get_user(httpctx_t* ctx) {
    int ok = 0;
    const char* id = query_param_char(ctx->request, "id", &ok);
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
    const char* slug = query_param_char(ctx->request, "slug", &ok);
    // slug = "my-first-post" for /api/posts/my-first-post
}
```

## Regular expressions

For complex patterns, use full regular expressions:

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

| Method | Description |
|--------|-------------|
| `GET` | Retrieve a resource |
| `POST` | Create a resource |
| `PUT` | Full update |
| `PATCH` | Partial update |
| `DELETE` | Delete a resource |
| `OPTIONS` | CORS preflight requests |

```json
"/api/resource": {
    "GET": { "file": "handlers/lib.so", "function": "get" },
    "POST": { "file": "handlers/lib.so", "function": "create" },
    "PUT": { "file": "handlers/lib.so", "function": "replace" },
    "PATCH": { "file": "handlers/lib.so", "function": "update" },
    "DELETE": { "file": "handlers/lib.so", "function": "delete" },
    "OPTIONS": { "file": "handlers/lib.so", "function": "options" }
}
```

## Middleware

Middleware are executed before the handler:

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

Request rate limiting:

```json
"ratelimits": {
    "default": { "burst": 15, "rate": 15 },
    "strict": { "burst": 1, "rate": 0 },
    "api": { "burst": 100, "rate": 100 }
},
"http": {
    "ratelimit": "default",
    "routes": {
        "/api/users": {
            "GET": {
                "file": "handlers/libapi.so",
                "function": "get_users",
                "ratelimit": "api"
            },
            "POST": {
                "file": "handlers/libapi.so",
                "function": "create_user",
                "ratelimit": "strict"
            }
        }
    }
}
```

## Redirects

```json
"http": {
    "redirects": {
        "/old-page": "/new-page",
        "/blog/(\\d+)": "/posts/{1}",
        "/user/(.*)": "/profile/{1}"
    }
}
```

Capture groups are available via `{1}`, `{2}`, etc.

## WebSocket routes

WebSocket uses a similar syntax:

```json
"websockets": {
    "default": {
        "file": "handlers/libws.so",
        "function": "ws_default"
    },
    "routes": {
        "/ws": {
            "GET": { "file": "handlers/libws.so", "function": "ws_connect" },
            "POST": { "file": "handlers/libws.so", "function": "ws_message" }
        },
        "/ws/chat/{room|\\d+}": {
            "GET": { "file": "handlers/libws.so", "function": "ws_join_room" },
            "POST": { "file": "handlers/libws.so", "function": "ws_send_message" }
        }
    }
}
```

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
    "routes": {
        "/api/v1/users": {
            "GET": { "file": "handlers/libusers.so", "function": "list" },
            "POST": { "file": "handlers/libusers.so", "function": "create" }
        },
        "/api/v1/users/{id|\\d+}": {
            "GET": { "file": "handlers/libusers.so", "function": "show" },
            "PUT": { "file": "handlers/libusers.so", "function": "update" },
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
