---
outline: deep
title: WebSocket Payload
description: Extracting data from WebSocket messages in C Web Framework. payload, payload_file and payload_json methods for reading the incoming frame body.
---

# Getting data from the client

The framework provides functions for extracting the body of an incoming WebSocket frame. The message body is streamed into a temporary file as the frame is received and read lazily — only when one of the functions below is called. The data format is arbitrary: text (including JSON) or raw binary bytes.

All functions take the connection protocol `ctx->request->protocol` and behave identically for text and binary frames.

## Functions overview

| Function | Returns | Purpose |
|----------|---------|---------|
| `websocketsrequest_payload` | `char*` | The entire frame body as a single string |
| `websocketsrequest_payload_file` | `file_content_t` | The frame body as a file descriptor (no copy) |
| `websocketsrequest_payload_json` | `json_doc_t*` | The frame body parsed as JSON |

::: tip Two ways to call
Each function is available in two equivalent forms: as a free function `websocketsrequest_payload(protocol)` and as a protocol method `protocol->get_payload(protocol)`. The protocol methods are the same functions bound to the object, so pick whichever fits your style. The free function takes the base `websockets_protocol_t*` and needs no cast.
:::

## websocketsrequest_payload

`char* websocketsrequest_payload(websockets_protocol_t* protocol);`

Reads the entire frame body into a null-terminated string. Convenient for text messages and raw binary data you want in a single block. Returns `NULL` when the body is empty.

```c
#include "websockets.h"

void echo(wsctx_t* ctx) {
    char* payload = websocketsrequest_payload(ctx->request->protocol);

    if (payload == NULL) {
        ctx->response->send_text(ctx->response, "Empty payload");
        return;
    }

    ctx->response->send_text(ctx->response, payload);

    free(payload);
}
```

> Returns a pointer to dynamically allocated memory. Always free it with `free()`.

## websocketsrequest_payload_file

`file_content_t websocketsrequest_payload_file(websockets_protocol_t* protocol);`

Returns the frame body as a lightweight `file_content_t` structure that **does not copy** the data — it references an offset inside the internal temporary file. This is the efficient way to handle large binary messages: copying happens only when you explicitly call `content()` or `make_file()`. The `ok` field is `0` when the body is empty.

```c
#include "websockets.h"

void upload(wsctx_t* ctx) {
    file_content_t payload = websocketsrequest_payload_file(ctx->request->protocol);

    if (!payload.ok) {
        ctx->response->send_text(ctx->response, "Empty payload");
        return;
    }

    // content() reads the bytes into an allocated buffer (freed via free).
    char* data = payload.content(&payload);
    if (data) {
        ctx->response->send_binaryn(ctx->response, data, payload.size);
        free(data);
    }
}
```

### The `file_content_t` structure

| Field / Method | Type | Description |
|----------------|------|-------------|
| `ok` | `int` | Success flag (`1` = data is valid) |
| `fd` | `int` | File descriptor of the internal temp file |
| `offset` | `off_t` | Offset where the data starts |
| `size` | `size_t` | Data size in bytes |
| `filename` | `char[NAME_MAX]` | File name; for WebSocket always `"tmpfile"` (a frame carries no name) |
| `make_file(dir, name)` | `file_t` | Save into directory `dir` (`name = NULL` → `"tmpfile"`) |
| `make_tmpfile(dir)` | `file_t` | Save into a temp file in `dir` (deleted on `close()`) |
| `content()` | `char*` | Read the bytes into memory (freed via `free`) |

::: warning File name
Unlike HTTP `multipart/form-data`, a WebSocket frame carries no file name, so `filename` is always `"tmpfile"`. When saving, pass the name explicitly via the second argument of `make_file(dir, name)`.
:::

## websocketsrequest_payload_json

`json_doc_t* websocketsrequest_payload_json(websockets_protocol_t* protocol);`

Parses the frame body as JSON and returns the document. Returns `NULL` on a parse error or an empty body. Equivalent to chaining `websocketsrequest_payload` → `json_parse`.

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

## Access via the protocol

Instead of the free functions, you can call the same methods through the connection protocol. This style is used in handlers that already obtained the protocol for other operations (for example, `get_query`). It requires a cast to the concrete protocol type — `websockets_protocol_resource_t` or `websockets_protocol_default_t`.

```c
#include "websockets.h"
#include "json.h"

void echo(wsctx_t* ctx) {
    websockets_protocol_resource_t* protocol = (websockets_protocol_resource_t*)ctx->request->protocol;

    // Try JSON first...
    json_doc_t* document = protocol->get_payload_json(protocol);
    if (document != NULL) {
        ctx->response->send_text(ctx->response, json_stringify(document));
        json_free(document);
        return;
    }

    // ...otherwise return as a raw string
    char* data = protocol->get_payload(protocol);
    if (data) {
        ctx->response->send_text(ctx->response, data);
        free(data);
        return;
    }

    ctx->response->send_text(ctx->response, "done");
}
```

| Protocol method | Equivalent function |
|-----------------|---------------------|
| `protocol->get_payload(protocol)` | `websocketsrequest_payload` |
| `protocol->get_payload_file(protocol)` | `websocketsrequest_payload_file` |
| `protocol->get_payload_json(protocol)` | `websocketsrequest_payload_json` |

## Memory management

- `websocketsrequest_payload` returns a dynamically allocated `char*` — free it with `free()`.
- `websocketsrequest_payload_json` returns a `json_doc_t*` — free it with `json_free()`.
- `file_content_t` from `websocketsrequest_payload_file` **does not own** the data — copying happens only when you call `content()` / `make_file()`. The buffer from `content()` is freed with `free()`, and the `file_t` from `make_file()` is closed with `close()`.
- Always check the result for `NULL` (string/JSON) or the `ok` field (`file_content_t`) before using it.
- Each frame's body lives only for the duration of request processing — do not keep pointers to it across handler calls.
