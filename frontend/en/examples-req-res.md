---
outline: deep
description: Examples of processing requests and responses using the Http and WebSockets protocols
---

# Request examples

## GET

**Route**

```json
// config.json
"/get": {
    "GET": { "file": "/var/www/server/build/exec/handlers/libindexpage.so", "function": "get" }
}
```

**request**

```curl
curl http://example.com/get \
    -X GET
```

**Handler**

```C
// handlers/indexpage.c
#include "http1.h"

void get(httpctx_t* ctx) {
    ctx->response->send_data(ctx->response, "Hello world!");
}
```

## POST payload

**Route**

```json
"/post": {
    "POST": { "file": "/var/www/server/build/exec/handlers/libindexpage.so", "function": "post" }
}
```

**request**

```curl
curl http://example.com/post \
    -X POST \
    -H 'Content-Type: text/plain' \
    -d 'Hello world'
```

**Handler**

```C
// handlers/indexpage.c
#include "http1.h"

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

## POST payloadf

**Route**

```json
"/post": {
    "POST": { "file": "/var/www/server/build/exec/handlers/libindexpage.so", "function": "post" }
}
```

**request**

```curl
curl http://example.com/post \
    -X POST \
    -F mydata=data \
    -F mytext=text
```

**Handler**

```C
// handlers/indexpage.c
#include "http1.h"

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

## POST payload_file

**Route**

```json
"/post": {
    "POST": { "file": "/var/www/server/build/exec/handlers/libindexpage.so", "function": "post" }
}
```

**request**

```curl
curl http://example.com/post \
    -X POST \
    -H 'Content-Type: text/plain' \
    --data-binary @/path/file.txt
```

**Handler**

```C
// handlers/indexpage.c
#include "http1.h"

void post(httpctx_t* ctx) {
    file_content_t payloadfile = ctx->request->get_payload_file(ctx->request);

    if (!payloadfile.ok) {
        ctx->response->send_data(ctx->response, "file not found");
        return;
    }

    if (!payloadfile.save(&payloadfile, "files", "file.txt")) {
        ctx->response->send_data(ctx->response, "Error save file");
        return;
    }

    char* data = payloadfile.read(&payloadfile);

    ctx->response->send_data(ctx->response, data);

    free(data);
}
```

## POST payload_filef

**Route**

```json
"/post": {
    "POST": { "file": "/var/www/server/build/exec/handlers/libindexpage.so", "function": "post" }
}
```

**request**

```curl
curl http://example.com/post \
    -X POST \
    -F myfile=@/path/file.txt
```

**Handler**

```C
// handlers/indexpage.c
#include "http1.h"

void post(httpctx_t* ctx) {
    file_content_t payloadfile = ctx->request->get_payload_filef(ctx->request, "myfile");

    if (!payloadfile.ok) {
        ctx->response->send_data(ctx->response, "file not found");
        return;
    }

    const char* filenameFromPayload = NULL;
    if (!payloadfile.save(&payloadfile, "files", filenameFromPayload)) {
        ctx->response->send_data(ctx->response, "Error save file");
        return;
    }

    char* data = payloadfile.read(&payloadfile);

    ctx->response->send_data(ctx->response, data);

    free(data);
}
```

## POST payload_json

**Route**

```json
"/post": {
    "POST": { "file": "/var/www/server/build/exec/handlers/libindexpage.so", "function": "post" }
}
```

**request**

```curl
curl http://example.com/post \
    -X POST \
    -H 'Content-Type: application/json' \
    -d '{ "key": "value" }'
```

**Handler**

```C
// handlers/indexpage.c
#include "http1.h"

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

    json_object_set(object, "mykey", json_create_string(document, "Hello"));

    ctx->response->add_header(ctx->response, "Content-Type", "application/json");

    ctx->response->send_data(ctx->response, json_stringify(document));

    json_free(document);
}
```

## POST payload_jsonf

**Route**

```json
"/post": {
    "POST": { "file": "/var/www/server/build/exec/handlers/libindexpage.so", "function": "post" }
}
```

**request**

```curl
curl http://example.com/post \
    -X POST \
    -F myjson='{ "key": "value" }'
```

**Handler**

```C
// handlers/indexpage.c
#include "http1.h"

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

    json_object_set(object, "mykey", json_create_string(document, "Hello"));

    ctx->response->add_header(ctx->response, "Content-Type", "application/json");

    ctx->response->send_data(ctx->response, json_stringify(document));

    json_free(document);
}
```

## Query

**Route**

```json
// config.json
"/query": {
    "GET": { "file": "/var/www/server/build/exec/handlers/libindexpage.so", "function": "query" }
},
"^/users/{id|\\d+}$": {
    "GET": { "file": "/var/www/server/build/exec/handlers/libindexpage.so", "function": "user" }
},
"^/params/{param1|\\d+}/{param2|[a-zA-Z0-9]+}$": {
    "GET": { "file": "/var/www/server/build/exec/handlers/libindexpage.so", "function": "params" }
}
```

**request**

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

```C
// handlers/indexpage.c
#include "http1.h"

void query(httpctx_t* ctx) {
    const char* data = ctx->request->query(ctx->request, "param");

    if (data) {
        ctx->response->send_data(ctx->response, data);
        return;
    }

    ctx->response->send_data(ctx->response, "Param not found");
}

void user(httpctx_t* ctx) {
    const char* data = ctx->request->query(ctx->request, "id");

    if (data) {
        ctx->response->send_data(ctx->response, data);
        return;
    }

    ctx->response->send_data(ctx->response, "User Id not found");
}

void params(httpctx_t* ctx) {
    const char* param1 = ctx->request->query(ctx->request, "param1");
    const char* param2 = ctx->request->query(ctx->request, "param2");

    if (param1 == NULL) {
        ctx->response->send_data(ctx->response, "Param1 not found");
        return;
    }

    if (param2 == NULL) {
        ctx->response->send_data(ctx->response, "Param2 not found");
        return;
    }

    ctx->response->send_data(ctx->response, param1);
}
```

## Reading cookies

**Route**

```json
"/cookie": {
    "GET": { "file": "/var/www/server/build/exec/handlers/libindexpage.so", "function": "cookie" }
}
```

**request**

```curl
curl http://example.com/cookie \
    -X GET
```

**Handler**

```C
// handlers/indexpage.c
#include "http1.h"

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

**Route**

```json
"/set_cookie": {
    "GET": { "file": "/var/www/server/build/exec/handlers/libindexpage.so", "function": "set_cookie" }
}
```

**request**

```curl
curl http://example.com/set_cookie \
    -X GET
```

**Handler**

```C
// handlers/indexpage.c
#include "http1.h"

void set_cookie(httpctx_t* ctx) {
    ctx->response->add_cookie(ctx->response, (cookie_t){
        .name = "token",
        .value = "token_value",
        .seconds = 3600,
        .path = "/",
        .domain = ".example.com"
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

**Route**

```json
"/old-resource": {
    "GET": { "file": "/var/www/server/build/exec/handlers/libindexpage.so", "function": "redirect" }
}
```

**request**

```curl
curl http://example.com/old-resource \
    -X GET
```

**Handler**

```C
// handlers/indexpage.c
#include "http1.h"

void redirect(httpctx_t* ctx) {
    ctx->response->redirect(ctx->response, "/new-resource", 301);
}
```

## Default response

**Route**

```json
"/default": {
    "GET": { "file": "/var/www/server/build/exec/handlers/libindexpage.so", "function": "def" }
}
```

**request**

```curl
curl http://example.com/default \
    -X GET
```

**Handler**

```C
// handlers/indexpage.c
#include "http1.h"

void def(httpctx_t* ctx) {
    ctx->response->send_default(ctx->response, 401);
}
```

## Reading headers

**Route**

```json
"/header": {
    "GET": { "file": "/var/www/server/build/exec/handlers/libindexpage.so", "function": "header" }
}
```

**request**

```curl
curl http://example.com/header \
    -X GET
```

**Handler**

```C
// handlers/indexpage.c
#include "http1.h"

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

**Route**

```json
"/set_header": {
    "GET": { "file": "/var/www/server/build/exec/handlers/libindexpage.so", "function": "set_header" }
}
```

**request**

```curl
curl http://example.com/set_header \
    -X GET
```

**Handler**

```C
// handlers/indexpage.c
#include "http1.h"

void set_header(httpctx_t* ctx) {
    ctx->response->add_header(ctx->response, "Content-Type", "text/plain");

    ctx->response->send_data(ctx->response, "Response with custom header");
}
```

## Send file

**Route**

```json
// config.json
"/file": {
    "GET": { "file": "/var/www/server/build/exec/handlers/libindexpage.so", "function": "file" }
}
```

**request**

```curl
curl http://example.com/file \
    -X GET
```

**Handler**

```C
// handlers/indexpage.c
#include "http1.h"

void file(httpctx_t* ctx) {
    ctx->response->send_file(ctx->response, "/files/file.txt");
}
```

## WebSocket connection

**Route**

```json
// config.json
"/wss": {
    "GET": { "file": "/var/www/server/build/exec/handlers/libindexpage.so", "function": "wss" }
}
```

**request**

```js
const socket = new WebSocket("wss://example.com/wss");
```

**Handler**

```C
// handlers/indexpage.c
#include "http1.h"
#include "websockets.h"

void wss(httpctx_t* ctx) {
    switch_to_websockets(ctx);
}
```

## WebSocket request

**Route**

```json
// config.json
"/wss_request": {
    "GET": { "file": "/var/www/server/build/exec/handlers/libwsindexpage.so", "function": "wss_request" }
}
```

**request**

```js
const socket = new WebSocket("wss://example.com/wss");

socket.onopen = (event) => {
	socket.send("GET /wss_request {\"key\": \"value\"}");
};
```

**Handler**

```C
// handlers/wsindexpage.c
#include "websockets.h"

void wss_request(wsctx_t* ctx) {
    const char* data = "Empty payload";

    if (ctx->request->payload)
        data = ctx->request->payload;

    ctx->response->send_text(ctx->response, data);
}
```

## WebSocket query

**Route**

```json
// config.json
"/wss_query": {
    "GET": { "file": "/var/www/server/build/exec/handlers/libwsindexpage.so", "function": "wss_query" }
}
```

**request**

```js
const socket = new WebSocket("wss://example.com/wss");

socket.onopen = (event) => {
	socket.send("GET /wss_query?q=text");
};
```

**Handler**

```C
// handlers/wsindexpage.c
#include "websockets.h"

void wss_query(wsctx_t* ctx) {
    const char* data = ctx->request->query(ctx->request, "q");

    if (!data)
        data = "Empty query";

    ctx->response->send_text(ctx->response, data);
}
```
