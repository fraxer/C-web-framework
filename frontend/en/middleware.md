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

## Global Middleware

Global middleware are configured in `config.json` and apply to all server routes:

```json
{
    "servers": {
        "s1": {
            "http": {
                "middlewares": [
                    "middleware_http_test_header",
                    "middleware_http_auth"
                ],
                "routes": { ... }
            }
        }
    }
}
```

### Registering Global Middleware

Global middleware are registered in the `middlewarelist.c` file:

```c
// app/middlewares/middlewarelist.c
#include "middleware_registry.h"
#include "httpmiddlewares.h"

middleware_global_fn_t middlewarelist[] = {
    { "middleware_http_test_header", middleware_http_test_header },
    { "middleware_http_auth", middleware_http_auth },
    { "", NULL }  // End of list
};
```

## Creating Middleware

### Simple Middleware

```c
// app/middlewares/httpmiddlewares.h
#ifndef __HTTPMIDDLEWARES__
#define __HTTPMIDDLEWARES__

#include "http.h"

int middleware_http_test_header(httpctx_t* ctx);
int middleware_http_auth(httpctx_t* ctx);

#endif
```

```c
// app/middlewares/httpmiddlewares.c
#include "httpmiddlewares.h"

int middleware_http_test_header(httpctx_t* ctx) {
    // Add header to all responses
    ctx->response->add_header(ctx->response, "X-Powered-By", "C Web Framework");
    return 1;  // Continue execution
}
```

### Authorization Middleware

```c
#include "httpmiddlewares.h"
#include "session.h"

int middleware_http_auth(httpctx_t* ctx) {
    const char* session_id = ctx->request->get_cookie(ctx->request, "session_id");
    if (session_id == NULL) {
        ctx->response->status_code = 401;
        ctx->response->send_data(ctx->response, "Unauthorized");
        return 0;  // Stop execution
    }

    char* session_data = session_get(session_id);
    if (session_data == NULL) {
        ctx->response->status_code = 401;
        ctx->response->send_data(ctx->response, "Session expired");
        return 0;
    }

    // Parse session data and set user
    json_doc_t* doc = json_parse(session_data);
    free(session_data);

    if (doc == NULL) {
        ctx->response->status_code = 401;
        ctx->response->send_data(ctx->response, "Invalid session");
        return 0;
    }

    json_token_t* root = json_root(doc);
    json_token_t* user_id_token = json_object_get(root, "user_id");

    int ok = 0;
    int user_id = json_int(user_id_token, &ok);
    json_free(doc);

    if (!ok || user_id < 1) {
        ctx->response->status_code = 401;
        ctx->response->send_data(ctx->response, "Invalid user");
        return 0;
    }

    // Load user and set in context
    array_t* params = array_create();
    mparams_fill_array(params, mparam_int(id, user_id));
    user_t* user = user_get(params);
    array_free(params);

    if (user == NULL) {
        ctx->response->status_code = 401;
        ctx->response->send_data(ctx->response, "User not found");
        return 0;
    }

    httpctx_set_user(ctx, user);
    return 1;  // Continue execution
}
```

### Middleware with 403 Response

```c
int middleware_http_forbidden(httpctx_t* ctx) {
    ctx->response->send_default(ctx->response, 403);
    return 0;
}
```

## Local Middleware (in Handler)

To apply middleware to a specific route, use the `middleware()` macro:

```c
#include "middleware.h"

void secret_page(httpctx_t* ctx) {
    // Check authorization before execution
    middleware(
        middleware_http_auth(ctx),
        middleware_check_role(ctx, "admin")
    );

    // Code will only execute if all middleware returned 1
    ctx->response->send_data(ctx->response, "Secret content");
}
```

The `middleware()` macro accepts any number of functions. If any of them returns 0, handler execution will stop.

### Role Check Example

```c
int middleware_check_role(httpctx_t* ctx, const char* required_role) {
    user_t* user = httpctx_get_user(ctx);
    if (user == NULL) {
        ctx->response->send_default(ctx->response, 401);
        return 0;
    }

    // Check user role
    if (!user_has_role(user, required_role)) {
        ctx->response->send_default(ctx->response, 403);
        return 0;
    }

    return 1;
}
```

## Middleware for Parameter Validation

```c
int middleware_http_query_param_required(httpctx_t* ctx, char** keys, int size) {
    char message[256] = {0};
    int ok = 0;

    for (int i = 0; i < size; i++) {
        const char* param = query_param_char(ctx->request, keys[i], &ok);
        if (!ok || param == NULL || param[0] == 0) {
            snprintf(message, sizeof(message), "Parameter \"%s\" is required", keys[i]);
            ctx->response->status_code = 400;
            ctx->response->send_data(ctx->response, message);
            return 0;
        }
    }

    return 1;
}

// Usage
void search(httpctx_t* ctx) {
    char* required[] = {"query", "page"};
    middleware(
        middleware_http_query_param_required(ctx, required, 2)
    );

    // Parameters are guaranteed to be present
    int ok;
    const char* query = query_param_char(ctx->request, "query", &ok);
    int page = query_param_int(ctx->request, "page", &ok);

    // ...
}
```

## Rate Limiting via Middleware

Rate limiting is configured in the configuration and applied automatically:

```json
{
    "servers": {
        "s1": {
            "ratelimits": {
                "strict": { "burst": 5, "rate": 1 },
                "normal": { "burst": 100, "rate": 50 }
            },
            "http": {
                "ratelimit": "normal",
                "routes": {
                    "/api/login": {
                        "POST": {
                            "file": "...",
                            "function": "login",
                            "ratelimit": "strict"
                        }
                    }
                }
            }
        }
    }
}
```

## WebSocket Middleware

Separate middleware are created for WebSocket connections:

```c
// app/middlewares/wsmiddlewares.h
#ifndef __WSMIDDLEWARES__
#define __WSMIDDLEWARES__

#include "websockets.h"

int middleware_ws_auth(wsctx_t* ctx);

#endif
```

```c
// app/middlewares/wsmiddlewares.c
#include "wsmiddlewares.h"
#include "session.h"

int middleware_ws_auth(wsctx_t* ctx) {
    // Authorization check for WebSocket
    // ...
    return 1;
}
```
