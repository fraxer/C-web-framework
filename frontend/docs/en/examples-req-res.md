---
outline: deep
description: Examples of processing requests and responses using the Http and WebSockets protocols
---

# Request examples

Ready-to-use examples for handling HTTP and WebSocket requests: reading parameters,
headers, cookies, and the request body, plus sending responses, files, and redirects.
For the full function reference, see [HTTP requests](/en/requests),
[HTTP responses](/en/responses), and [Request data](/en/payload).

## GET

Returns a string as the response body via `send_data`:

**Route**

```json
// config.json
"/get": {
    "GET": { "file": "handlers/libindexpage.so", "function": "get" }
}
```

**Request**

```curl
curl http://example.com/get \
    -X GET
```

**Handler**

```c
// handlers/indexpage.c
#include "http.h"

void get(httpctx_t* ctx) {
    ctx->response->send_data(ctx->response, "Hello world!");
}
```

## POST get_payload

`get_payload` returns the entire request body as a single string. It fits `text/plain`
and raw data; the result must be released with `free`:

**Route**

```json
"/post": {
    "POST": { "file": "handlers/libindexpage.so", "function": "post" }
}
```

**Request**

```curl
curl http://example.com/post \
    -X POST \
    -H 'Content-Type: text/plain' \
    -d 'Hello world'
```

**Handler**

```c
// handlers/indexpage.c
#include "http.h"

void post(httpctx_t* ctx) {
    char* payload = ctx->request->get_payload(ctx->request);

    if (payload == NULL) {
        ctx->response->send_data(ctx->response, "Payload not found");
        return;
    }

    ctx->response->send_data(ctx->response, payload);

    free(payload);
}
```

## POST get_payloadf

`get_payloadf` extracts a single named field from `multipart/form-data` or
`application/x-www-form-urlencoded`:

**Route**

```json
"/post": {
    "POST": { "file": "handlers/libindexpage.so", "function": "post" }
}
```

**Request**

```curl
curl http://example.com/post \
    -X POST \
    -F mydata=data \
    -F mytext=text
```

**Handler**

```c
// handlers/indexpage.c
#include "http.h"

void post(httpctx_t* ctx) {
    char* data = ctx->request->get_payloadf(ctx->request, "mydata");
    char* text = ctx->request->get_payloadf(ctx->request, "mytext");

    if (data == NULL) {
        ctx->response->send_data(ctx->response, "Data not found");
        goto failed;
    }

    if (text == NULL) {
        ctx->response->send_data(ctx->response, "Text not found");
        goto failed;
    }

    ctx->response->send_data(ctx->response, text);

    failed:

    if (data) free(data);
    if (text) free(text);
}
```

## POST get_payload_file

`get_payload_file` returns an uploaded file as a `file_content_t`. The `make_file(dir, name)`
method writes it to disk (pass `NULL` as the name to keep the original), and `content`
reads the bytes into memory. Always check the `.ok` flag:

**Route**

```json
"/post": {
    "POST": { "file": "handlers/libindexpage.so", "function": "post" }
}
```

**Request**

```curl
curl http://example.com/post \
    -X POST \
    -H 'Content-Type: text/plain' \
    --data-binary @/path/file.txt
```

**Handler**

```c
// handlers/indexpage.c
#include "http.h"

void post(httpctx_t* ctx) {
    file_content_t payloadfile = ctx->request->get_payload_file(ctx->request);

    if (!payloadfile.ok) {
        ctx->response->send_data(ctx->response, "file not found");
        return;
    }

    // make_file(dir, name) writes the data to disk and returns an open file_t
    file_t saved = payloadfile.make_file(&payloadfile, "files", "file.txt");
    if (!saved.ok) {
        ctx->response->send_data(ctx->response, "Error save file");
        return;
    }
    saved.close(&saved);

    // content reads the bytes into an allocated buffer (freed with free)
    char* data = payloadfile.content(&payloadfile);

    ctx->response->send_data(ctx->response, data);

    free(data);
}
```

## POST get_payload_filef

Same as `get_payload_file`, but the file is selected by its field name from
`multipart/form-data`:

**Route**

```json
"/post": {
    "POST": { "file": "handlers/libindexpage.so", "function": "post" }
}
```

**Request**

```curl
curl http://example.com/post \
    -X POST \
    -F myfile=@/path/file.txt
```

**Handler**

```c
// handlers/indexpage.c
#include "http.h"

void post(httpctx_t* ctx) {
    file_content_t payloadfile = ctx->request->get_payload_filef(ctx->request, "myfile");

    if (!payloadfile.ok) {
        ctx->response->send_data(ctx->response, "file not found");
        return;
    }

    // NULL keeps the original filename from the request
    file_t saved = payloadfile.make_file(&payloadfile, "files", NULL);
    if (!saved.ok) {
        ctx->response->send_data(ctx->response, "Error save file");
        return;
    }
    saved.close(&saved);

    char* data = payloadfile.content(&payloadfile);

    ctx->response->send_data(ctx->response, data);

    free(data);
}
```

## POST get_payload_json

`get_payload_json` parses the request body (`application/json`) and returns a
`json_doc_t*`. Validate it with `json_ok`:

**Route**

```json
"/post": {
    "POST": { "file": "handlers/libindexpage.so", "function": "post" }
}
```

**Request**

```curl
curl http://example.com/post \
    -X POST \
    -H 'Content-Type: application/json' \
    -d '{ "key": "value" }'
```

**Handler**

```c
// handlers/indexpage.c
#include "http.h"
#include "json.h"

void post(httpctx_t* ctx) {
    json_doc_t* document = ctx->request->get_payload_json(ctx->request);

    if (!json_ok(document)) {
        ctx->response->send_data(ctx->response, json_error(document));
        json_free(document);
        return;
    }

    json_token_t* object = json_root(document);
    if (!json_is_object(object)) {
        ctx->response->send_data(ctx->response, "is not object");
        return;
    }

    json_object_set(object, "mykey", json_create_string("Hello"));

    ctx->response->add_header(ctx->response, "Content-Type", "application/json");

    ctx->response->send_data(ctx->response, json_stringify(document));

    json_free(document);
}
```

## POST get_payload_jsonf

Parses JSON from a single `multipart/form-data` field by key:

**Route**

```json
"/post": {
    "POST": { "file": "handlers/libindexpage.so", "function": "post" }
}
```

**Request**

```curl
curl http://example.com/post \
    -X POST \
    -F myjson='{ "key": "value" }'
```

**Handler**

```c
// handlers/indexpage.c
#include "http.h"
#include "json.h"

void post(httpctx_t* ctx) {
    json_doc_t* document = ctx->request->get_payload_jsonf(ctx->request, "myjson");

    if (!json_ok(document)) {
        ctx->response->send_data(ctx->response, json_error(document));
        json_free(document);
        return;
    }

    json_token_t* object = json_root(document);
    if (!json_is_object(object)) {
        ctx->response->send_data(ctx->response, "is not object");
        return;
    }

    json_object_set(object, "mykey", json_create_string("Hello"));

    ctx->response->add_header(ctx->response, "Content-Type", "application/json");

    ctx->response->send_data(ctx->response, json_stringify(document));

    json_free(document);
}
```

## Query

Query string parameters and route parameters are read the same way — with
`query_param_char` from `query.h`, against the same `ctx->request->query_`. The `ok`
out-parameter tells "parameter is missing" apart from an empty value:

**Route**

```json
// config.json
"/query": {
    "GET": { "file": "handlers/libindexpage.so", "function": "query" }
},
"^/users/{id|\\d+}$": {
    "GET": { "file": "handlers/libindexpage.so", "function": "user" }
},
"^/params/{param1|\\d+}/{param2|[a-zA-Z0-9]+}$": {
    "GET": { "file": "handlers/libindexpage.so", "function": "params" }
}
```

**Request**

```curl
curl http://example.com/query?param=text \
    -X GET
```

```curl
curl http://example.com/users/123 \
    -X GET
```

```curl
curl http://example.com/params/100/param_value \
    -X GET
```

**Handler**

```c
// handlers/indexpage.c
#include "http.h"
#include "query.h"

void query(httpctx_t* ctx) {
    int ok = 0;
    const char* data = query_param_char(ctx->request->query_, "param", &ok);

    if (ok) {
        ctx->response->send_data(ctx->response, data);
        return;
    }

    ctx->response->send_data(ctx->response, "Param not found");
}

void user(httpctx_t* ctx) {
    int ok = 0;
    const char* data = query_param_char(ctx->request->query_, "id", &ok);

    if (ok) {
        ctx->response->send_data(ctx->response, data);
        return;
    }

    ctx->response->send_data(ctx->response, "User Id not found");
}

void params(httpctx_t* ctx) {
    int ok = 0;
    const char* param1 = query_param_char(ctx->request->query_, "param1", &ok);

    if (!ok) {
        ctx->response->send_data(ctx->response, "Param1 not found");
        return;
    }

    const char* param2 = query_param_char(ctx->request->query_, "param2", &ok);
    if (!ok) {
        ctx->response->send_data(ctx->response, "Param2 not found");
        return;
    }

    ctx->response->send_data(ctx->response, param1);
}
```

## Reading cookies

`get_cookie` returns a cookie value by name, or `NULL`:

**Route**

```json
"/cookie": {
    "GET": { "file": "handlers/libindexpage.so", "function": "cookie" }
}
```

**Request**

```curl
curl http://example.com/cookie \
    -X GET
```

**Handler**

```c
// handlers/indexpage.c
#include "http.h"

void cookie(httpctx_t* ctx) {
    const char* token = ctx->request->get_cookie(ctx->request, "token");

    if (token == NULL) {
        ctx->response->send_data(ctx->response, "Token not found");
        return;
    }

    ctx->response->send_data(ctx->response, token);
}
```

## Set cookie

`add_cookie` adds a `Set-Cookie` header through a `cookie_t` struct:

**Route**

```json
"/set_cookie": {
    "GET": { "file": "handlers/libindexpage.so", "function": "set_cookie" }
}
```

**Request**

```curl
curl http://example.com/set_cookie \
    -X GET
```

**Handler**

```c
// handlers/indexpage.c
#include "http.h"

void set_cookie(httpctx_t* ctx) {
    ctx->response->add_cookie(ctx->response, (cookie_t){
        .name = "token",
        .value = "token_value",
        .seconds = 3600,
        .path = "/",
        .domain = ".example.com",
        .secure = 1,
        .http_only = 1,
        .same_site = "Lax"
    });

    ctx->response->add_cookie(ctx->response, (cookie_t){
        .name = "timestamp",
        .value = "123456789",
        .seconds = 3600
    });

    ctx->response->send_data(ctx->response, "Cookie added");
}
```

## Redirect

`redirect(url, code)` sends a redirect with the given status code:

**Route**

```json
"/old-resource": {
    "GET": { "file": "handlers/libindexpage.so", "function": "redirect" }
}
```

**Request**

```curl
curl http://example.com/old-resource \
    -X GET
```

**Handler**

```c
// handlers/indexpage.c
#include "http.h"

void redirect(httpctx_t* ctx) {
    ctx->response->redirect(ctx->response, "/new-resource", 301);
}
```

## Default response

`send_default(code)` sends a standard response with the given status code and no body:

**Route**

```json
"/default": {
    "GET": { "file": "handlers/libindexpage.so", "function": "def" }
}
```

**Request**

```curl
curl http://example.com/default \
    -X GET
```

**Handler**

```c
// handlers/indexpage.c
#include "http.h"

void def(httpctx_t* ctx) {
    ctx->response->send_default(ctx->response, 401);
}
```

## Reading headers

`get_header` returns an `http_header_t*` struct, or `NULL`:

**Route**

```json
"/header": {
    "GET": { "file": "handlers/libindexpage.so", "function": "header" }
}
```

**Request**

```curl
curl http://example.com/header \
    -X GET
```

**Handler**

```c
// handlers/indexpage.c
#include "http.h"

void header(httpctx_t* ctx) {
    http_header_t* header_host = ctx->request->get_header(ctx->request, "Host");

    if (header_host) {
        ctx->response->send_data(ctx->response, header_host->value);
        return;
    }

    ctx->response->send_data(ctx->response, "header Host not found");
}
```

## Set headers

`add_header(key, value)` adds a response header:

**Route**

```json
"/set_header": {
    "GET": { "file": "handlers/libindexpage.so", "function": "set_header" }
}
```

**Request**

```curl
curl http://example.com/set_header \
    -X GET
```

**Handler**

```c
// handlers/indexpage.c
#include "http.h"

void set_header(httpctx_t* ctx) {
    ctx->response->add_header(ctx->response, "Content-Type", "text/plain");

    ctx->response->send_data(ctx->response, "Response with custom header");
}
```

## Send file

`send_file(path)` sends a file to the client:

**Route**

```json
// config.json
"/file": {
    "GET": { "file": "handlers/libindexpage.so", "function": "file" }
}
```

**Request**

```curl
curl http://example.com/file \
    -X GET
```

**Handler**

```c
// handlers/indexpage.c
#include "http.h"

void file(httpctx_t* ctx) {
    ctx->response->send_file(ctx->response, "/files/file.txt");
}
```

## WebSocket connection

`switch_to_websockets` upgrades the HTTP connection to a WebSocket. Subsequent messages
are handled by `wsctx_t*` WebSocket handlers:

**Route**

```json
// config.json
"/wss": {
    "GET": { "file": "handlers/libindexpage.so", "function": "wss" }
}
```

**Request**

```js
const socket = new WebSocket("wss://example.com/wss", "resource");
```

**Handler**

```c
// handlers/indexpage.c
#include "http.h"
#include "websockets.h"

void wss(httpctx_t* ctx) {
    switch_to_websockets(ctx);
}
```

## WebSocket request

The text body of a message is read with `websocketsrequest_payload(protocol)` and sent
back through `send_text`:

**Route**

```json
// config.json
"/wss_request": {
    "GET": { "file": "handlers/libwsindexpage.so", "function": "wss_request" }
}
```

**Request**

```js
const socket = new WebSocket("wss://example.com/wss", "resource");

socket.onopen = (event) => {
    socket.send("GET /wss_request {\"key\": \"value\"}");
};
```

**Handler**

```c
// handlers/wsindexpage.c
#include "websockets.h"

void wss_request(wsctx_t* ctx) {
    char* payload = websocketsrequest_payload(ctx->request->protocol);
    const char* data = payload ? payload : "Empty payload";

    ctx->response->send_text(ctx->response, data);

    if (payload) free(payload);
}
```

## WebSocket query

WebSocket request parameters are available through `protocol->query_`. The protocol
struct must be cast to `websockets_protocol_resource_t*` — the same `query_param_char`
then reads both query string and route parameters:

**Route**

```json
// config.json
"/wss_query": {
    "GET": { "file": "handlers/libwsindexpage.so", "function": "wss_query" }
}
```

**Request**

```js
const socket = new WebSocket("wss://example.com/wss", "resource");

socket.onopen = (event) => {
    socket.send("GET /wss_query?q=text");
};
```

**Handler**

```c
// handlers/wsindexpage.c
#include "websockets.h"
#include "query.h"

void wss_query(wsctx_t* ctx) {
    websockets_protocol_resource_t* protocol = (websockets_protocol_resource_t*)ctx->request->protocol;

    int ok = 0;
    const char* data = query_param_char(protocol->query_, "q", &ok);

    if (!ok)
        data = "Empty query";

    ctx->response->send_text(ctx->response, data);
}
```
