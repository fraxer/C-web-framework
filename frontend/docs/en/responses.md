---
outline: deep
description: Forming HTTP responses in C Web Framework. Status codes, headers, JSON, redirects, sending files and models.
---

# HTTP responses

HTTP responses are formed through the `response` object in the `httpctx_t` context. A response contains a status code, headers, and body.

## Status code

By default, the status code is `200`. To change it:

```c
void handler(httpctx_t* ctx) {
    ctx->response->statusCode = 201; // Created
    ctx->response->send_data(ctx->response, "Resource created");
}
```

### Standard responses

The `send_default()` method sends a standard response with the specified code:

```c
void handler(httpctx_t* ctx) {
    if (error) {
        ctx->response->send_default(ctx->response, 500); // Internal Server Error
        return;
    }

    if (!found) {
        ctx->response->send_default(ctx->response, 404); // Not Found
        return;
    }

    ctx->response->send_data(ctx->response, "Success");
}
```

Full list: [HTTP codes](/http-codes)

## HTTP headers

Adding response headers:

```c
void handler(httpctx_t* ctx) {
    ctx->response->add_header(ctx->response, "Content-Type", "application/json");
    ctx->response->add_header(ctx->response, "X-Custom-Header", "value");

    ctx->response->send_data(ctx->response, "{\"status\": \"ok\"}");
}
```

## Sending data

### Text data

```c
void handler(httpctx_t* ctx) {
    ctx->response->send_data(ctx->response, "Hello World!");
}
```

### JSON response

```c
#include "json.h"

void handler(httpctx_t* ctx) {
    json_doc_t* doc = json_init();
    json_token_t* root = json_create_object(doc);

    json_object_set(root, "status", json_create_string(doc, "success"));
    json_object_set(root, "code", json_create_int(doc, 200));

    ctx->response->add_header(ctx->response, "Content-Type", "application/json");
    ctx->response->send_data(ctx->response, json_stringify(doc));

    json_free(doc);
}
```

### Sending a model

Models can be sent directly with field selection:

```c
#include "user.h"

void get_user(httpctx_t* ctx) {
    user_t* user = user_find_by_id(123);

    if (!user) {
        ctx->response->send_default(ctx->response, 404);
        return;
    }

    // Send model as JSON with selected fields
    ctx->response->send_model(ctx->response, user,
                              display_fields("id", "email", "name"));

    user_free(user);
}
```

## Redirects

Redirecting to another URL:

```c
void handler(httpctx_t* ctx) {
    // 301 — permanent redirect
    ctx->response->redirect(ctx->response, "/new-location", 301);
}

void temp_redirect(httpctx_t* ctx) {
    // 302 — temporary redirect
    ctx->response->redirect(ctx->response, "/temporary", 302);
}
```

## Sending files

### Static file

```c
void download(httpctx_t* ctx) {
    ctx->response->send_file(ctx->response, "/path/to/file.pdf");
}
```

### File from storage

```c
#include "storage.h"

void download_from_storage(httpctx_t* ctx) {
    file_t file = storage_file_get("local", "/uploads/document.pdf");

    if (!file.ok) {
        ctx->response->send_default(ctx->response, 404);
        return;
    }

    ctx->response->send_filef(ctx->response, "local", "/uploads/%s", "document.pdf");
    file.close(&file);
}
```

## Cookie

Setting cookies in the response:

```c
void login(httpctx_t* ctx) {
    // ... validate credentials ...

    ctx->response->add_cookie(ctx->response, (cookie_t){
        .name = "session_token",
        .value = "abc123xyz",
        .seconds = 3600,         // Lifetime in seconds
        .path = "/",
        .domain = ".example.com",
        .secure = 1,             // HTTPS only
        .http_only = 1,          // Not accessible from JavaScript
        .same_site = "Lax"       // CSRF protection
    });

    ctx->response->send_data(ctx->response, "Logged in");
}
```

Deleting a cookie:

```c
void logout(httpctx_t* ctx) {
    ctx->response->add_cookie(ctx->response, (cookie_t){
        .name = "session_token",
        .value = "",
        .seconds = -1,  // Expired time
        .path = "/"
    });

    ctx->response->redirect(ctx->response, "/login", 302);
}
```

## CORS headers

```c
void api_handler(httpctx_t* ctx) {
    ctx->response->add_header(ctx->response, "Access-Control-Allow-Origin", "*");
    ctx->response->add_header(ctx->response, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE");
    ctx->response->add_header(ctx->response, "Access-Control-Allow-Headers", "Content-Type, Authorization");

    if (ctx->request->method == ROUTE_OPTIONS) {
        ctx->response->statusCode = 204;
        ctx->response->send_data(ctx->response, "");
        return;
    }

    // Main logic...
}
```

## Important

- The response is sent to the client only after calling `send_data()`, `send_file()`, `send_model()`, or `redirect()`
- Call one of these methods at the end of each handler
- After sending a response, the handler should terminate (use `return`)
- Do not call `send_data()` after `send_file()` — this will result in an incorrect response
