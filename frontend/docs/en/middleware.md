---
outline: deep
description: Middleware in C Web Framework. Creating handler chains, authorization checks, request validation.
---

# Middleware

Middleware are intermediate handlers that execute before the main route handler. They are used for authentication, validation, logging, and other tasks.

## How It Works

Middleware is a function that takes a request context and returns:

- `1` — continue chain execution
- `0` — stop execution (main handler will not be called)

```c
typedef int (*middleware_fn_p)(void*);
```

When returning `0`, the middleware itself is responsible for producing a response (status, body) — e.g. via `ctx->response->send_data()` or `send_default()`. Returning `1` simply passes control forward.

## Global Middleware

Global middleware are configured in `config.json` and apply to all HTTP server routes. The names in the array refer to middleware registered in the registry (see below):

```json
{
    "servers": {
        "s1": {
            "http": {
                "middlewares": [
                    "middleware_http_test_header"
                ],
                "routes": { ... }
            }
        }
    }
}
```

### Registering in the Registry

Global middleware are registered in `app/middlewares/middlewarelist.c`, inside the `middlewares_init()` function, via `middleware_registry_register(name, fn)`. This function is called during application initialization (from the `moduleloader`):

```c
// app/middlewares/middlewarelist.c
#include "middleware_registry.h"
#include "httpmiddlewares.h"
#include "wsmiddlewares.h"
#include "log.h"

int middlewares_init(void) {
    if (!middleware_registry_register("middleware_http_forbidden", (middleware_fn_p)middleware_http_forbidden)) {
        log_error("middlewares_init: failed to register middleware_http_forbidden\n");
        return 0;
    }

    if (!middleware_registry_register("middleware_http_test_header", (middleware_fn_p)middleware_http_test_header)) {
        log_error("middlewares_init: failed to register middleware_http_test_header\n");
        return 0;
    }

    // Add new middleware here:
    // middleware_registry_register("middleware_cors", (middleware_fn_p)middleware_cors);

    return 1;
}
```

::: tip Name = config.json key
The string passed as the first argument to `middleware_registry_register()` is exactly the name referenced in the configuration `middlewares` array.
:::

### Registry API

The registry interface is declared in `core/framework/middleware/middleware_registry.h`:

| Function | Purpose |
| --- | --- |
| `middlewares_init()` | Registers all application middleware at startup. |
| `middleware_registry_register(name, handler)` | Registers a middleware by name. `1` on success, `0` on error (full / duplicate). |
| `middleware_by_name(name)` | Returns the middleware function by name, or `NULL`. |
| `middleware_registry_get_all(&count)` | Returns the array of all registered middleware. |
| `middleware_registry_clear()` | Clears the registry (used during config reload). |

## Creating HTTP Middleware

HTTP middleware take `httpctx_t*`. The header includes `httpctx.h` and `middleware.h`:

```c
// app/middlewares/httpmiddlewares.h
#ifndef __HTTPMIDDLEWARES__
#define __HTTPMIDDLEWARES__

#include "httpctx.h"
#include "middleware.h"

int middleware_http_forbidden(httpctx_t* ctx);
int middleware_http_test_header(httpctx_t* ctx);
int middleware_http_query_param_required(httpctx_t* ctx, char** keys, int size);
int middleware_http_auth(httpctx_t* ctx);

#endif
```

### Simple Middleware

```c
// app/middlewares/httpmiddlewares.c
#include "httpmiddlewares.h"

int middleware_http_test_header(httpctx_t* ctx) {
    ctx->response->add_header(ctx->response, "X-Test-Header", "test");
    return 1;  // Continue execution
}
```

### Denying Access (403)

```c
int middleware_http_forbidden(httpctx_t* ctx) {
    ctx->response->send_default(ctx->response, 403);
    return 0;  // Stop execution
}
```

### Authorization Middleware

Loads the user from the session and makes it available in the context via `httpctx_set_user()`:

```c
#include "httpmiddlewares.h"
#include "session.h"

int middleware_http_auth(httpctx_t* ctx) {
    const char* session_id = ctx->request->get_cookie(ctx->request, "session_id");
    if (session_id == NULL) {
        ctx->response->send_data(ctx->response, "Session id not found");
        return 0;
    }

    int result = 0;
    json_doc_t* document = NULL;

    // First argument is the session name from config.json ("sessions" section)
    char* session_data = session_get("backend", session_id);
    if (session_data == NULL) {
        ctx->response->send_data(ctx->response, "Session not found");
        return 0;
    }

    document = json_parse(session_data);
    if (document == NULL) {
        ctx->response->send_data(ctx->response, "");
        goto failed;
    }

    json_token_t* object = json_root(document);
    if (!json_is_object(object)) {
        ctx->response->send_data(ctx->response, "Session data is not object");
        goto failed;
    }

    json_token_t* token_user_id = json_object_get(object, "user_id");
    if (!json_is_number(token_user_id)) {
        ctx->response->send_data(ctx->response, "Session data is not valid");
        goto failed;
    }

    int ok = 0;
    const int user_id = json_int(token_user_id, &ok);
    if (!ok || user_id < 1) {
        ctx->response->send_data(ctx->response, "User is not valid");
        goto failed;
    }

    array_t* params = array_create();
    mparams_fill_array(params, mparam_int(id, user_id));
    user_t* user = user_get(params);
    array_free(params);

    if (user == NULL) {
        ctx->response->send_data(ctx->response, "User not found");
        goto failed;
    }

    httpctx_set_user(ctx, user);
    result = 1;

failed:
    free(session_data);
    json_free(document);
    return result;
}
```

To read the user inside a handler, use the `httpctx_get_user(ctx)` macro — it is cast to `user_t*`:

```c
user_t* user = httpctx_get_user(ctx);
```

## Local Middleware (in Handler)

To apply middleware to a specific route, use the `middleware()` macro. It accepts any number of calls and expands to a short-circuit:

```c
#define middleware(...) if (!(MW_ITEM(MW_VA_NARGS(__VA_ARGS__), __VA_ARGS__))) return;
// expansion for two arguments:
// if (!(fn1(ctx) && fn2(ctx))) return;
```

If any middleware returns `0`, the handler returns immediately. Therefore the macro can **only be used inside `void` functions** (route handlers).

```c
// app/routes/auth/auth.c
void secret_page(httpctx_t* ctx) {
    // The code below runs only if the middleware returned 1
    middleware(
        middleware_http_auth(ctx)
    );

    ctx->response->send_data(ctx->response, "done");
}
```

Multiple middleware are separated by commas and executed left to right until the first `0`:

```c
void admin_page(httpctx_t* ctx) {
    middleware(
        middleware_http_auth(ctx),
        middleware_check_role(ctx, "admin")
    );

    ctx->response->send_data(ctx->response, "admin area");
}
```

### Your Own Handler Middleware

Such middleware do not need to be registered in the registry — just declare them and call via the macro. Example of a role check:

```c
int middleware_check_role(httpctx_t* ctx, const char* required_role) {
    user_t* user = httpctx_get_user(ctx);
    if (user == NULL) {
        ctx->response->send_default(ctx->response, 401);
        return 0;
    }

    if (!user_has_role(user, required_role)) {  // your role check
        ctx->response->send_default(ctx->response, 403);
        return 0;
    }

    return 1;
}
```

## Parameter Validation

`middleware_http_query_param_required()` validates the presence of required query parameters. To pass the list of keys, the `args_str(...)` macro from `core/misc/macros.h` is handy — it builds a string array right from the arguments:

```c
int middleware_http_query_param_required(httpctx_t* ctx, char** keys, int size) {
    char message[256] = {0};
    int ok = 0;

    for (int i = 0; i < size; i++) {
        const char* param = query_param_char(ctx->request->query_, keys[i], &ok);
        if (!ok) {
            return 0;
        }

        if (param == NULL || param[0] == 0) {
            sprintf(message, "param \"%s\" not found", keys[i]);
            ctx->response->send_data(ctx->response, message);
            return 0;
        }
    }

    return 1;
}
```

Usage in a handler:

```c
// app/routes/middleware/middleware.c
void example(httpctx_t* ctx) {
    middleware(
        middleware_http_query_param_required(ctx, args_str("a", "abc"))
    );

    // Parameters are guaranteed to be present
    ctx->response->send_data(ctx->response, "done");
}
```

An equivalent form without the macro — a plain `if`:

```c
if (!middleware_http_query_param_required(ctx, args_str("a", "abc"))) return;
```

## WebSocket Middleware

For WebSocket connections, separate middleware are created with the signature `int name(wsctx_t* ctx, ...)`. The header includes `wsctx.h` and `middleware.h`:

```c
// app/middlewares/wsmiddlewares.h
#ifndef __WSMIDDLEWARES__
#define __WSMIDDLEWARES__

#include "wsctx.h"
#include "middleware.h"

int middleware_ws_query_param_required(wsctx_t* ctx, char** keys, int size);

#endif
```

Access to handshake query parameters goes through the protocol resource:

```c
// app/middlewares/wsmiddlewares.c
#include "websockets.h"
#include "wsmiddlewares.h"

int middleware_ws_query_param_required(wsctx_t* ctx, char** keys, int size) {
    websockets_protocol_resource_t* protocol = (websockets_protocol_resource_t*)ctx->request->protocol;
    char message[256] = {0};

    for (int i = 0; i < size; i++) {
        const char* param = protocol->get_query(protocol, keys[i]);
        if (param == NULL || param[0] == 0) {
            sprintf(message, "param <%s> not found", keys[i]);
            ctx->response->send_data(ctx, message);
            return 0;
        }
    }

    return 1;
}
```

::: warning WebSocket middleware are local only
WS middleware are not registered globally in `middlewares_init()`. They are applied locally — via the `middleware()` macro inside the WebSocket handler itself:

```c
// app/routes/ws/wsindex.c
void middleware_test(wsctx_t* ctx) {
    middleware(
        middleware_ws_query_param_required(ctx, args_str("abc"))
    );

    ctx->response->send_data(ctx, "done");
}
```
:::

## Rate Limiting

Request rate limiting is configured separately — in `config.json`, not through the middleware system. Named buckets are declared in `servers.<id>.ratelimits`, and binding happens at the HTTP server level (`ratelimit`) or on a specific route:

```json
{
    "servers": {
        "s1": {
            "ratelimits": {
                "one":  { "burst": 1,  "rate": 0 },
                "two":  { "burst": 15, "rate": 15 }
            },
            "http": {
                "ratelimit": "one",
                "routes": {
                    "/api/login": {
                        "POST": {
                            "file": "...",
                            "function": "login",
                            "ratelimit": "two"
                        }
                    }
                }
            }
        }
    }
}
```

- `burst` — bucket size (maximum number of instantaneous requests);
- `rate` — bucket refill rate (requests per second); `0` means no refill.
