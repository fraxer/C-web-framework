---
outline: deep
description: Processing HTTP requests in C Web Framework. Request structure, method, URL, query parameters, headers, cookies, body and JSON.
---

# HTTP requests

HTTP requests are processed through the `httpctx_t` context. It holds pointers to the request and response objects plus a slot for arbitrary user data.

```c
typedef struct httpctx {
    httprequest_t* request;   // request object
    httpresponse_t* response; // response object
    void* user_data;          // arbitrary data (e.g. authenticated user)
} httpctx_t;
```

## Basic handler structure

```c
#include "http.h"

void my_handler(httpctx_t* ctx) {
    ctx->response->send_data(ctx->response, "Hello World");
}
```

Request and response methods are function pointers attached to the object. The first argument is always the object itself: `ctx->request->get_header(ctx->request, "Host")`.

## Request fields

Main fields of the `httprequest_t` object:

| Field | Type | Description |
|-------|------|-------------|
| `uri` | `const char*` | Full URI with query string: `/users?id=100` |
| `path` | `const char*` | Path without query string: `/users` |
| `uri_length` | `size_t` | Length of `uri` |
| `path_length` | `size_t` | Length of `path` |
| `method` | `route_methods_e` | HTTP request method |
| `version` | `http_version_e` | Protocol version (HTTP/1.0, HTTP/1.1) |
| `query_` | `query_t*` | Linked list of query and route parameters |

```c
void handler(httpctx_t* ctx) {
    // Limit URI length
    if (ctx->request->uri_length > 4096) {
        ctx->response->send_default(ctx->response, 414); // URI Too Long
        return;
    }

    ctx->response->send_data(ctx->response, ctx->request->path);
}
```

## HTTP method

The current method is available through `ctx->request->method` and is represented by the `route_methods_e` enum values (`core/src/route/route.h`):

```c
void handler(httpctx_t* ctx) {
    switch (ctx->request->method) {
        case ROUTE_GET:     // GET
        case ROUTE_HEAD:    // HEAD
        case ROUTE_POST:    // POST
        case ROUTE_PUT:     // PUT
        case ROUTE_PATCH:   // PATCH
        case ROUTE_DELETE:  // DELETE
        case ROUTE_OPTIONS: // OPTIONS
            break;
        case ROUTE_NONE:    // method not set
            break;
    }
}
```

## Query parameters

Query string parameters (after `?`) are available through the `ctx->request->query_` field and the functions from `query.h`. Besides the string `query_param_char`, there are typed extractors for numbers and collections:

```c
#include "query.h"

void list_users(httpctx_t* ctx) {
    int ok = 0;

    int page = query_param_int(ctx->request->query_, "page", &ok);
    if (!ok) page = 1; // default value

    const char* q = query_param_char(ctx->request->query_, "q", &ok);
    if (!ok) q = NULL;

    // ...
}
```

::: tip Check ok for numeric params
`0` can be either a valid value or an error indicator (missing param or wrong format). For numeric parameters the `ok` check is mandatory. The full list of functions (`int`, `double`, arrays, objects) is in the [Query parameters](/en/query-params) section.
:::

## Route parameters

Dynamic URL segments are declared in `config.json` with the `{name|pattern}` format and end up in the same `query_` list:

```json
// config.json
"/api/users/{id|\\d+}": {
    "GET": { "file": "handlers/libapi.so", "function": "get_user" }
}
```

```c
#include "query.h"

void get_user(httpctx_t* ctx) {
    int ok = 0;
    const char* id = query_param_char(ctx->request->query_, "id", &ok);
    // for /api/users/123 id == "123"
    if (!ok) {
        ctx->response->send_default(ctx->response, 400);
        return;
    }
}
```

More on routing rules: [Routing](/en/routing).

## Request body

Body data is extracted through the `get_payload*` methods on the request object. Supported formats are `multipart/form-data`, `application/x-www-form-urlencoded`, `application/json`, and arbitrary bodies.

### JSON

```c
#include "json.h"

void post_json(httpctx_t* ctx) {
    json_doc_t* doc = ctx->request->get_payload_json(ctx->request);

    if (!json_ok(doc)) {
        ctx->response->send_data(ctx->response, "Invalid JSON");
        json_free(doc);
        return;
    }

    json_token_t* root = json_root(doc);
    json_token_t* name = json_object_get(root, "name");

    if (name && json_is_string(name)) {
        ctx->response->send_data(ctx->response, json_string(name));
    }

    json_free(doc);
}
```

### Form field

For `multipart/form-data` and `application/x-www-form-urlencoded`, a field value is fetched by key. The returned string must be freed with `free()`:

```c
void post_form(httpctx_t* ctx) {
    char* email = ctx->request->get_payloadf(ctx->request, "email");
    if (email == NULL) {
        ctx->response->send_data(ctx->response, "Missing field");
        return;
    }

    ctx->response->send_data(ctx->response, email);
    free(email);
}
```

A full overview of the methods (`get_payload`, `get_payloadf`, `get_payload_file*`, `get_payload_jsonf`) and working with uploaded files: [Getting data from the client](/en/payload).

## HTTP headers

`get_header` looks up a header by name (case-insensitive) and returns a pointer to an `http_header_t` node or `NULL`:

```c
typedef struct http_header {
    char*  key;
    char*  value;
    size_t key_length;
    size_t value_length;
    struct http_header* next;
} http_header_t;
```

```c
void handler(httpctx_t* ctx) {
    http_header_t* host = ctx->request->get_header(ctx->request, "Host");
    http_header_t* auth = ctx->request->get_header(ctx->request, "Authorization");

    if (host) {
        printf("Host: %s\n", host->value);
    }

    if (auth) {
        // verify token...
    }
}
```

Related methods:

| Method | Description |
|--------|-------------|
| `get_header(name)` | Header by name (null-terminated) |
| `get_headern(name, length)` | Lookup by name with explicit length |
| `add_header(name, value)` | Add a header |
| `add_headern(name, name_len, value, value_len)` | Add with explicit lengths |
| `remove_header(name)` | Remove a header |

## Cookie

`get_cookie` returns the cookie **value** directly as a string (not a struct) or `NULL`:

```c
void handler(httpctx_t* ctx) {
    const char* token = ctx->request->get_cookie(ctx->request, "session_token");

    if (token == NULL) {
        ctx->response->send_data(ctx->response, "Not authorized");
        return;
    }

    // validate token...
}
```

More on setting and reading cookies: [Cookie](/en/cookie).

## user_data

The `ctx->user_data` field holds an arbitrary pointer and is handy for passing data between middleware and a handler — for example, the authenticated user. The example app provides typed helpers for it:

```c
httpctx_set_user(ctx, user);          // set
user_t* user = httpctx_get_user(ctx); // get
```

## Database

Database access goes through `dbquery` with a host identifier. The `<driver>.<host_id>` form selects a specific host; `<driver>` without a `host_id` spreads requests evenly (round-robin) across all hosts of that driver:

```c
#include "db.h"

void handler(httpctx_t* ctx) {
    dbresult_t* result = dbquery("postgresql.p1", "SELECT 1", NULL);

    if (!dbresult_ok(result)) {
        ctx->response->send_data(ctx->response, "Database not available");
        dbresult_free(result);
        return;
    }

    // queries...
    dbresult_free(result);
}
```

More details: [Database](/en/db).
