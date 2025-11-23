---
outline: deep
title: WebSockets receiving data from the client
description: Cody provides methods for retrieving data via WebSockets protocol as is or in json format
---

# Getting data from the client

Cpdy provides several methods for extracting data from a request.

### payload

`char*(*payload)(struct websocketsrequest*);`

Retrieves all available data from the request body as a string.

Returns a pointer of type `char` to dynamically allocated memory.

After working with the data, be sure to free the memory.

```C
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

### payload_json

`json_doc_t*(*payload_json)(struct websocketsrequest*);`

Extracts all available data from the request body and creates a json document `json_doc_t`.

Returns a pointer of type `json_doc_t`.

After working with the json document, you need to free the memory.

```C
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
