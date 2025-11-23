---
outline: deep
description: Примеры обработки запросов и ответов по протоколам Http и WebSockets
---

# Примеры запросов

## GET

**Маршрут**

```json
// config.json
"/get": {
    "GET": { "file": "handlers/libindexpage.so", "function": "get" }
}
```

**Запрос**

```curl
curl http://example.com/get \
    -X GET
```

**Обработчик**

```c
// handlers/indexpage.c
#include "http.h"

void get(httpctx_t* ctx) {
    ctx->response->send_data(ctx->response, "Hello world!");
}
```

## POST get_payload

**Маршрут**

```json
"/post": {
    "POST": { "file": "handlers/libindexpage.so", "function": "post" }
}
```

**Запрос**

```curl
curl http://example.com/post \
    -X POST \
    -H 'Content-Type: text/plain' \
    -d 'Hello world'
```

**Обработчик**

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

**Маршрут**

```json
"/post": {
    "POST": { "file": "handlers/libindexpage.so", "function": "post" }
}
```

**Запрос**

```curl
curl http://example.com/post \
    -X POST \
    -F mydata=data \
    -F mytext=text
```

**Обработчик**

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

**Маршрут**

```json
"/post": {
    "POST": { "file": "handlers/libindexpage.so", "function": "post" }
}
```

**Запрос**

```curl
curl http://example.com/post \
    -X POST \
    -H 'Content-Type: text/plain' \
    --data-binary @/path/file.txt
```

**Обработчик**

```c
// handlers/indexpage.c
#include "http.h"

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

## POST get_payload_filef

**Маршрут**

```json
"/post": {
    "POST": { "file": "handlers/libindexpage.so", "function": "post" }
}
```

**Запрос**

```curl
curl http://example.com/post \
    -X POST \
    -F myfile=@/path/file.txt
```

**Обработчик**

```c
// handlers/indexpage.c
#include "http.h"

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

## POST get_payload_json

**Маршрут**

```json
"/post": {
    "POST": { "file": "handlers/libindexpage.so", "function": "post" }
}
```

**Запрос**

```curl
curl http://example.com/post \
    -X POST \
    -H 'Content-Type: application/json' \
    -d '{ "key": "value" }'
```

**Обработчик**

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

    json_object_set(object, "mykey", json_create_string(document, "Hello"));

    ctx->response->add_header(ctx->response, "Content-Type", "application/json");

    ctx->response->send_data(ctx->response, json_stringify(document));

    json_free(document);
}
```

## POST get_payload_jsonf

**Маршрут**

```json
"/post": {
    "POST": { "file": "handlers/libindexpage.so", "function": "post" }
}
```

**Запрос**

```curl
curl http://example.com/post \
    -X POST \
    -F myjson='{ "key": "value" }'
```

**Обработчик**

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

    json_object_set(object, "mykey", json_create_string(document, "Hello"));

    ctx->response->add_header(ctx->response, "Content-Type", "application/json");

    ctx->response->send_data(ctx->response, json_stringify(document));

    json_free(document);
}
```

## Query

**Маршрут**

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

**Запрос**

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

**Обработчик**

```c
// handlers/indexpage.c
#include "http.h"
#include "query.h"

void query(httpctx_t* ctx) {
    int ok = 0;
    const char* data = query_param_char(ctx->request, "param", &ok);

    if (ok) {
        ctx->response->send_data(ctx->response, data);
        return;
    }

    ctx->response->send_data(ctx->response, "Param not found");
}

void user(httpctx_t* ctx) {
    int ok = 0;
    const char* data = query_param_char(ctx->request, "id", &ok);

    if (ok) {
        ctx->response->send_data(ctx->response, data);
        return;
    }

    ctx->response->send_data(ctx->response, "User Id not found");
}

void params(httpctx_t* ctx) {
    int ok = 0;
    const char* param1 = query_param_char(ctx->request, "param1", &ok);

    if (!ok) {
        ctx->response->send_data(ctx->response, "Param1 not found");
        return;
    }

    const char* param2 = query_param_char(ctx->request, "param2", &ok);
    if (!ok) {
        ctx->response->send_data(ctx->response, "Param2 not found");
        return;
    }

    ctx->response->send_data(ctx->response, param1);
}
```

## Чтение cookie

**Маршрут**

```json
"/cookie": {
    "GET": { "file": "handlers/libindexpage.so", "function": "cookie" }
}
```

**Запрос**

```curl
curl http://example.com/cookie \
    -X GET
```

**Обработчик**

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

## Установка cookie

**Маршрут**

```json
"/set_cookie": {
    "GET": { "file": "handlers/libindexpage.so", "function": "set_cookie" }
}
```

**Запрос**

```curl
curl http://example.com/set_cookie \
    -X GET
```

**Обработчик**

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

## Редирект

**Маршрут**

```json
"/old-resource": {
    "GET": { "file": "handlers/libindexpage.so", "function": "redirect" }
}
```

**Запрос**

```curl
curl http://example.com/old-resource \
    -X GET
```

**Обработчик**

```c
// handlers/indexpage.c
#include "http.h"

void redirect(httpctx_t* ctx) {
    ctx->response->redirect(ctx->response, "/new-resource", 301);
}
```

## Стандартный ответ

**Маршрут**

```json
"/default": {
    "GET": { "file": "handlers/libindexpage.so", "function": "def" }
}
```

**Запрос**

```curl
curl http://example.com/default \
    -X GET
```

**Обработчик**

```c
// handlers/indexpage.c
#include "http.h"

void def(httpctx_t* ctx) {
    ctx->response->send_default(ctx->response, 401);
}
```

## Чтение заголовков

**Маршрут**

```json
"/header": {
    "GET": { "file": "handlers/libindexpage.so", "function": "header" }
}
```

**Запрос**

```curl
curl http://example.com/header \
    -X GET
```

**Обработчик**

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

## Установка заголовков

**Маршрут**

```json
"/set_header": {
    "GET": { "file": "handlers/libindexpage.so", "function": "set_header" }
}
```

**Запрос**

```curl
curl http://example.com/set_header \
    -X GET
```

**Обработчик**

```c
// handlers/indexpage.c
#include "http.h"

void set_header(httpctx_t* ctx) {
    ctx->response->add_header(ctx->response, "Content-Type", "text/plain");

    ctx->response->send_data(ctx->response, "Response with custom header");
}
```

## Отправка файла

**Маршрут**

```json
// config.json
"/file": {
    "GET": { "file": "handlers/libindexpage.so", "function": "file" }
}
```

**Запрос**

```curl
curl http://example.com/file \
    -X GET
```

**Обработчик**

```c
// handlers/indexpage.c
#include "http.h"

void file(httpctx_t* ctx) {
    ctx->response->send_file(ctx->response, "/files/file.txt");
}
```

## WebSocket соединение

**Маршрут**

```json
// config.json
"/wss": {
    "GET": { "file": "handlers/libindexpage.so", "function": "wss" }
}
```

**Запрос**

```js
const socket = new WebSocket("wss://example.com/wss", "resource");
```

**Обработчик**

```c
// handlers/indexpage.c
#include "http.h"
#include "websockets.h"

void wss(httpctx_t* ctx) {
    switch_to_websockets(ctx->request, ctx->response);
}
```

## WebSocket запрос

**Маршрут**

```json
// config.json
"/wss_request": {
    "GET": { "file": "handlers/libwsindexpage.so", "function": "wss_request" }
}
```

**Запрос**

```js
const socket = new WebSocket("wss://example.com/wss", "resource");

socket.onopen = (event) => {
	socket.send("GET /wss_request {\"key\": \"value\"}");
};
```

**Обработчик**

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

**Маршрут**

```json
// config.json
"/wss_query": {
    "GET": { "file": "handlers/libwsindexpage.so", "function": "wss_query" }
}
```

**Запрос**

```js
const socket = new WebSocket("wss://example.com/wss", "resource");

socket.onopen = (event) => {
	socket.send("GET /wss_query?q=text");
};
```

**Обработчик**

```c
// handlers/wsindexpage.c
#include "websockets.h"
#include "query.h"

void wss_query(wsctx_t* ctx) {
    int ok = 0;
    const char* data = query_param_char(ctx->request, "q", &ok);

    if (!ok)
        data = "Empty query";

    ctx->response->send_text(ctx->response, data);
}
```
