---
outline: deep
title: WebSocket Payload
description: Извлечение данных из WebSocket-сообщений в C Web Framework. Методы payload и payload_json для обработки входящих данных.
---

# Получение данных от клиента

Фреймворк предоставляет методы для извлечения данных из WebSocket-сообщений.

### websocketsrequest_payload

`char* websocketsrequest_payload(websockets_protocol_t* protocol);`

Извлекает все доступные данные из тела запроса в виде строки.

Возвращает указатель типа `char` на динамически выделенную память.

После работы с данными обязательно освободите память.

```c
#include "websockets.h"

void post(wsctx_t* ctx) {
    char* payload = websocketsrequest_payload(ctx->request->protocol);

    if (payload == NULL) {
        ctx->response->send_text(ctx->response, "Payload not found");
        return;
    }

    ctx->response->send_text(ctx->response, payload);

    free(payload);
}
```

### websocketsrequest_payload_json

`json_doc_t* websocketsrequest_payload_json(websockets_protocol_t* protocol);`

Извлекает все доступные данные из тела запроса и создает json-документ `json_doc_t`.

Возвращает указатель типа `json_doc_t`.

После работы с json-документом необходимо освобождать память.

```c
#include "websockets.h"
#include "json.h"

void post(wsctx_t* ctx) {
    json_doc_t* document = websocketsrequest_payload_json(ctx->request->protocol);

    if (!json_ok(document)) {
        ctx->response->send_text(ctx->response, json_error(document));
        goto failed;
    }

    json_token_t* object = json_root(document);
    if (!json_is_object(object)) {
        ctx->response->send_text(ctx->response, "is not object");
        goto failed;
    }

    json_object_set(object, "mykey", json_create_string(document, "Hello"));

    ctx->response->send_text(ctx->response, json_stringify(document));

    failed:

    json_free(document);
}
```
