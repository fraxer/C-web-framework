---
outline: deep
description: Forming HTTP responses in C Web Framework. Status code, headers, sending data, JSON, models and views, redirects, files and cookies.
---

# HTTP responses

HTTP responses are formed through the `httpctx_t->response` object (`httpresponse_t*`). As with the request, response methods are function pointers attached to the object, so the first argument is always the object itself: `ctx->response->send_data(ctx->response, "...")`.

```c
#include "http.h"

void my_handler(httpctx_t* ctx) {
    ctx->response->send_data(ctx->response, "Hello World");
}
```

A response holds a status code, headers, and a body. The body is produced by one of the `send_*` methods — nothing is sent to the client until one of them is called.

## Status code

The `status_code` field holds the current code (`200` by default). Change it by direct assignment before sending the body:

```c
void handler(httpctx_t* ctx) {
    ctx->response->status_code = 201; // Created
    ctx->response->send_data(ctx->response, "Resource created");
}
```

### Standard response

`send_default()` sends a standard response with the given code and a generated HTML page — handy for errors:

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

Full list of codes: [HTTP codes](/en/http-codes).

## HTTP headers

Headers are added before the body is sent. `add_header` takes null-terminated strings, the `n` variant takes explicit lengths, and `add_headeru` adds a header only if it is not already set (unique):

```c
void handler(httpctx_t* ctx) {
    ctx->response->add_header(ctx->response, "X-Custom-Header", "value");
    ctx->response->send_data(ctx->response, "ok");
}
```

Related methods:

| Method | Description |
|--------|-------------|
| `add_header(key, value)` | Add a header (null-terminated) |
| `add_headern(key, key_len, value, value_len)` | Add with explicit lengths |
| `add_headeru(key, key_len, value, value_len)` | Add only if not already set |
| `add_content_length(length)` | Add `Content-Length` |
| `get_header(key)` | Get a header node by name (case-insensitive) or `NULL` |
| `remove_header(key)` | Remove a header |

::: tip Content-Type is set by the send method
The `send_*` methods set `Content-Type`, `Connection`, and `Cache-Control` themselves via `add_headeru`. To override `Content-Type`, add your own header **before** calling `send_*` — the default value will not be duplicated.
:::

## Sending data

`send_data` sends a null-terminated string, `send_datan` sends a buffer with an explicit length (binary-safe). Both set `Content-Type: text/html; charset=utf-8`:

```c
void handler(httpctx_t* ctx) {
    ctx->response->send_data(ctx->response, "Hello World!");
}

void raw(httpctx_t* ctx) {
    const char data[] = { 0x00, 0x01, 0x02, 0x03 };
    ctx->response->send_datan(ctx->response, data, sizeof(data));
}
```

## JSON

`send_json` serializes a ready `json_doc_t*` and sets `Content-Type: application/json`. The caller frees the document:

```c
#include "json.h"

void handler(httpctx_t* ctx) {
    json_doc_t* doc = json_init();
    json_token_t* root = json_create_object(doc);

    json_object_set(root, "status", json_create_string(doc, "success"));
    json_object_set(root, "code", json_create_int(doc, 200));

    ctx->response->send_json(ctx->response, doc);

    json_free(doc);
}
```

More on building documents: [JSON](/en/json).

## Models

An ORM model is serialized to JSON directly via `send_model`, an array of models via `send_models`. The set of fields to display is set with the `display_fields(...)` macro from `mparams.h`; `NULL` means all model fields.

```c
#include "user.h"
#include "mparams.h"

void get_user(httpctx_t* ctx) {
    user_t* user = user_get(ctx); // example of fetching a model

    if (user == NULL) {
        ctx->response->send_default(ctx->response, 404);
        return;
    }

    // Only the specified fields
    ctx->response->send_model(ctx->response, user,
                              display_fields("id", "email", "name"));
}
```

An array of models (for listings, for example):

```c
void list_users(httpctx_t* ctx) {
    array_t* users = user_list(ctx); // returns array_t*

    if (users == NULL) {
        ctx->response->send_default(ctx->response, 500);
        return;
    }

    ctx->response->send_models(ctx->response, users,
                               display_fields("id", "email", "name"));
}
```

::: tip The display_fields macro
`display_fields(...)` expands to a `char*[]` with a terminating `NULL`, so it accepts any number of field names: `display_fields("id", "name")`, or a single `NULL` for all fields.
:::

Model definitions and CRUD wrappers: [Models](/en/model).

## Views

`send_view` renders a template (view) from a named storage. The first argument is a `json_doc_t*` with data for the template (can be `NULL`), followed by the storage name and a path with `printf`-style formatting:

```c
void index(httpctx_t* ctx) {
    json_doc_t* document = json_init();
    json_token_t* root = json_create_object(document);
    json_object_set(root, "title", json_create_string(document, "Home"));

    ctx->response->send_view(ctx->response, document, "views", "/index.tpl");

    json_free(document);
}
```

On template syntax: [Views](/en/view).

## Redirects

`redirect` adds the `Location` header and sets the code:

```c
void handler(httpctx_t* ctx) {
    ctx->response->redirect(ctx->response, "/new-location", 301); // permanent
}

void temp(httpctx_t* ctx) {
    ctx->response->redirect(ctx->response, "/temporary", 302); // temporary
}
```

Codes `301`, `302`, `307`, `308` are supported. External links (`http://`/`https://`) are detected automatically; `Connection: Close` is added for them.

## Files

`send_file` sends a file by path relative to the server root, `send_filen` with an explicit path length. `Content-Type` is detected by extension and range requests (Range) are supported:

```c
void download(httpctx_t* ctx) {
    ctx->response->send_file(ctx->response, "files/report.pdf");
}
```

`send_filef` reads a file from a named storage with `printf`-style path formatting (see `storages` in `config.json`):

```c
void from_storage(httpctx_t* ctx) {
    ctx->response->send_filef(ctx->response, "local", "/uploads/%s", "document.pdf");
}
```

If the file is missing or not accessible, the methods send `404`/`403` themselves. More on storages: [Storage](/en/storage).

## Cookies

`add_cookie` adds a `Set-Cookie` from a `cookie_t` structure:

```c
typedef struct {
    const char* name;      /* name (required) */
    const char* value;     /* value (required) */
    int seconds;           /* lifetime: >0 — Expires, 0 — Max-Age=0 (deletion) */
    const char* path;      /* Path (NULL to skip) */
    const char* domain;    /* Domain (NULL to skip) */
    int secure;            /* 1 — HTTPS only */
    int http_only;         /* 1 — not accessible from JavaScript */
    const char* same_site; /* "Strict" | "Lax" | "None" (NULL to skip) */
} cookie_t;
```

Setting a cookie on login:

```c
void login(httpctx_t* ctx) {
    // ... validate credentials ...

    ctx->response->add_cookie(ctx->response, (cookie_t){
        .name = "session_token",
        .value = "abc123xyz",
        .seconds = 3600,
        .path = "/",
        .domain = ".example.com",
        .secure = 1,
        .http_only = 1,
        .same_site = "Lax"
    });

    ctx->response->send_data(ctx->response, "Logged in");
}
```

Deleting a cookie — empty value and `seconds = 0` (sets `Max-Age=0`):

```c
void logout(httpctx_t* ctx) {
    ctx->response->add_cookie(ctx->response, (cookie_t){
        .name = "session_token",
        .value = "",
        .seconds = 0,
        .path = "/"
    });

    ctx->response->redirect(ctx->response, "/login", 302);
}
```

Reading cookies from the request: [Cookie](/en/cookie).

## CORS

CORS headers are added as ordinary headers via `add_header`:

```c
void api_handler(httpctx_t* ctx) {
    ctx->response->add_header(ctx->response, "Access-Control-Allow-Origin", "*");
    ctx->response->add_header(ctx->response, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE");
    ctx->response->add_header(ctx->response, "Access-Control-Allow-Headers", "Content-Type, Authorization");

    if (ctx->request->method == ROUTE_OPTIONS) {
        ctx->response->status_code = 204;
        ctx->response->send_data(ctx->response, "");
        return;
    }

    // main logic...
}
```

## Important

- Nothing is sent to the client until one of `send_*` (`send_data`, `send_datan`, `send_json`, `send_model`, `send_models`, `send_view`, `send_file`, `send_filef`, `send_default`) or `redirect` is called.
- After sending a response, the handler must terminate (`return`) — sending again produces an incorrect response.
- `send_default`, `send_json`, and `send_model*` set `Content-Type` themselves; do not duplicate it.
- `send_file`/`send_filef` return a standard code when a file is missing, so no separate check is needed.
