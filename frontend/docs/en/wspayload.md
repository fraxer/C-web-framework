---
outline: deep
title: WebSocket Payload
description: Extracting data from WebSocket messages in C Web Framework. payload and payload_json methods for processing incoming data.
---

# Getting data from the client

The framework provides methods for extracting data from WebSocket messages.

### websocketsrequest_payload

`char* websocketsrequest_payload(websockets_protocol_t* protocol);`

Extracts all available data from the request body as a string.

Returns a pointer of type `char` to dynamically allocated memory.

After working with the data, be sure to free the memory.

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

Extracts all available data from the request body and creates a json document `json_doc_t`.

Returns a pointer of type `json_doc_t`.

After working with the json document, you need to free the memory.

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
