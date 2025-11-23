---
outline: deep
description: Processing HTTP requests in C Web Framework. Extracting parameters, cookies, headers, working with payload and JSON.
---

# HTTP requests

HTTP requests are processed through the `httpctx_t` context, which provides access to `request` and `response` objects.

## Basic handler structure

```c
#include "http.h"

void my_handler(httpctx_t* ctx) {
    // ctx->request — request object
    // ctx->response — response object

    ctx->response->send_data(ctx->response, "Hello World");
}
```

## GET request parameters

Query string parameters are extracted using functions from `query.h`:

```c
#include "query.h"

void get_users(httpctx_t* ctx) {
    int ok_id = 0, ok_name = 0;
    const char* id = query_param_char(ctx->request, "id", &ok_id);
    const char* name = query_param_char(ctx->request, "name", &ok_name);

    if (ok_id && ok_name) {
        ctx->response->send_data(ctx->response, id);
        return;
    }

    ctx->response->send_data(ctx->response, "Missing parameters");
}
```

## Route parameters

Dynamic URL parameters are also available through functions from `query.h`:

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
    const char* id = query_param_char(ctx->request, "id", &ok);
    // id contains the value from URL, e.g. "123" for /api/users/123
    if (!ok) {
        ctx->response->send_data(ctx->response, "Invalid id");
    }
}
```

## POST/PUT/PATCH request data

Request body data is available through payload methods:

### Getting the entire request body

```c
void post_data(httpctx_t* ctx) {
    char* payload = ctx->request->get_payload(ctx->request);

    if (!payload) {
        ctx->response->send_data(ctx->response, "No payload");
        return;
    }

    ctx->response->send_data(ctx->response, payload);
    free(payload);
}
```

### Getting a field by key

For `multipart/form-data` and `application/x-www-form-urlencoded`:

```c
void post_form(httpctx_t* ctx) {
    char* email = ctx->request->get_payloadf(ctx->request, "email");
    char* password = ctx->request->get_payloadf(ctx->request, "password");

    if (!email || !password) {
        ctx->response->send_data(ctx->response, "Missing fields");
        if (email) free(email);
        if (password) free(password);
        return;
    }

    // Process data...

    free(email);
    free(password);
}
```

### Getting JSON from request body

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

More about working with payload: [Getting data from the client](/payload)

## HTTP request method

The current method is available through `ctx->request->method`:

```c
void handler(httpctx_t* ctx) {
    switch (ctx->request->method) {
        case ROUTE_GET:
            // GET request
            break;
        case ROUTE_POST:
            // POST request
            break;
        case ROUTE_PUT:
            // PUT request
            break;
        case ROUTE_PATCH:
            // PATCH request
            break;
        case ROUTE_DELETE:
            // DELETE request
            break;
        case ROUTE_OPTIONS:
            // OPTIONS request
            break;
    }
}
```

## Request URL

Properties for working with URLs:

| Property | Description | Example |
|----------|-------------|---------|
| `uri` | Full URI with parameters | `/users?id=100` |
| `path` | Path without parameters | `/users` |
| `ext` | File extension | `html` |
| `uri_length` | URI length | — |
| `path_length` | Path length | — |
| `ext_length` | Extension length | — |

```c
void handler(httpctx_t* ctx) {
    // Check URI length
    if (ctx->request->uri_length > 4096) {
        ctx->response->send_default(ctx->response, 414); // URI Too Long
        return;
    }

    ctx->response->send_data(ctx->response, ctx->request->path);
}
```

## HTTP headers

Getting request headers:

```c
void handler(httpctx_t* ctx) {
    http_header_t* host = ctx->request->get_header(ctx->request, "Host");
    http_header_t* auth = ctx->request->get_header(ctx->request, "Authorization");

    if (host) {
        printf("Host: %s\n", host->value);
    }

    if (auth) {
        // Check authorization...
    }
}
```

## Cookie

Reading cookies from request:

```c
void handler(httpctx_t* ctx) {
    const char* token = ctx->request->get_cookie(ctx->request, "session_token");

    if (!token) {
        ctx->response->send_data(ctx->response, "Not authorized");
        return;
    }

    // Validate token...
}
```

More details: [Cookie](/cookie)

## Database connection

Accessing databases through context:

```c
#include "db.h"

void handler(httpctx_t* ctx) {
    dbresult_t* result = dbquery("postgresql", "SELECT 1", NULL);

    if (!dbresult_ok(result)) {
        ctx->response->send_data(ctx->response, "Database not available");
        dbresult_free(result);
        return;
    }

    // Execute queries...
    dbresult_free(result);
}
```

More details: [Database](/db)
