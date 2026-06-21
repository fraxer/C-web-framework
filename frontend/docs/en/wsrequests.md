---
outline: deep
title: WebSocket requests
description: Processing WebSocket requests in C Web Framework. The wsctx_t context, resource protocol, method, URL, query parameters, and frame body.
---

# WebSocket requests

WebSocket provides a persistent bidirectional connection between the client and server, allowing real-time data exchange without HTTP overhead. With its API, you can send a message to the server and receive a response without making an HTTP request, and this process is event-driven.

## Request context

WebSocket requests are handled through the `wsctx_t` context. It holds pointers to the request and response objects and a slot for arbitrary user data.

```c
typedef struct wsctx {
    websocketsrequest_t* request;   // request object
    websocketsresponse_t* response; // response object
    void* user_data;                // arbitrary data (e.g. the authenticated user)
} wsctx_t;
```

The `websocketsrequest_t` object stores the frame's bookkeeping fields (type, fragmentation, compression) and a reference to a **protocol**. The protocol is what parses the HTTP-like message and keeps the method, path, query parameters, and body:

```c
typedef struct websocketsrequest {
    request_t base;                  // reset / free
    websockets_datatype_e type;      // frame type: TEXT, BINARY, PING, PONG, CLOSE
    websockets_protocol_t* protocol; // parsing and routing protocol
    int can_reset;
    int fragmented;                  // message spans multiple frames
    int compressed;                  // permessage-deflate compressed
    connection_t* connection;        // connection this request belongs to
} websocketsrequest_t;
```

Request data is accessed through the protocol. The framework ships two variants: `websockets_protocol_resource_t` (HTTP-like routing `METHOD /path?query DATA`) and a simpler `websockets_protocol_default_t` (frame body only, no address or method). This page covers the resource protocol — the one used by `config.json` routes.

## Basic handler structure

```c
#include "websockets.h"

void my_handler(wsctx_t* ctx) {
    ctx->response->send_text(ctx->response, "Hello World");
}
```

The request and response methods are function pointers bound to the object. The first argument is always the object itself: `ctx->response->send_text(ctx->response, ...)`.

## Request format

Resource-protocol messages have the form `[method] [path] [payload]`. The method, path, and payload are separated by a single space, for example:

```
GET /
GET /users?id=100
POST /users {"id": 100, "name":"Alex"}
PATCH /users/100 {"name":"Alen"}
DELETE /users/100
```

Recognized methods are `GET`, `POST`, `PATCH`, `DELETE`. A message with any other method is rejected.

## HTTP method

The current method is available through the `protocol->method` field after casting the protocol to `websockets_protocol_resource_t`, and is represented by the values of the `route_methods_e` enum (`core/src/route/route.h`):

```c
#include "websockets.h"

void handler(wsctx_t* ctx) {
    websockets_protocol_resource_t* protocol = (websockets_protocol_resource_t*)ctx->request->protocol;

    switch (protocol->method) {
        case ROUTE_GET:    // GET
        case ROUTE_POST:   // POST
        case ROUTE_PATCH:  // PATCH
        case ROUTE_DELETE: // DELETE
            break;
        case ROUTE_NONE:   // method not set
            break;
    }
}
```

::: tip The method lives on the protocol
The `method` field belongs to the resource protocol, not to the request object itself. `websockets_protocol_default_t` has no method or path — it only deals with the frame body.
:::

## Request URL

The resource protocol splits the address into components:

| Field | Type | Description |
|-------|------|-------------|
| `uri` | `char*` | Raw URI with the query string: `/users?id=100` |
| `path` | `char*` | URL-decoded path without the query string: `/users` |
| `uri_length` | `size_t` | Length of `uri` |
| `path_length` | `size_t` | Length of `path` |
| `query_` | `query_t*` | Linked list of query parameters and route parameters |

```c
void handler(wsctx_t* ctx) {
    websockets_protocol_resource_t* protocol = (websockets_protocol_resource_t*)ctx->request->protocol;

    ctx->response->send_text(ctx->response, protocol->path);
}
```

::: tip Ready-made lengths
The `uri` and `path` fields are null-terminated, so `strlen()` works, but `uri_length` and `path_length` come straight from the parser and avoid scanning the string.
:::

## Query parameters

Query-string parameters (after `?`) and route parameters land in the shared `protocol->query_` list. The shortest way to read a value is the protocol's `get_query` method, which returns the string or `NULL` when the parameter is absent:

```c
#include "websockets.h"

void get(wsctx_t* ctx) {
    websockets_protocol_resource_t* protocol = (websockets_protocol_resource_t*)ctx->request->protocol;
    const char* data = protocol->get_query(protocol, "mydata");

    if (data) {
        ctx->response->send_text(ctx->response, data);
        return;
    }

    ctx->response->send_text(ctx->response, "Data not found");
}
```

For type-safe extraction — integers, floating point, arrays — use the functions from `query.h` directly against the `protocol->query_` list, the same way you would in an HTTP handler. See [Query parameters](/en/query-params).

## Request body

The body is carried by `POST` and `PATCH` methods. Extraction is done with the `websocketsrequest_payload*` functions or the protocol methods of the same name. A full overview (including file and JSON handling) is in [Receiving data from the client](/en/wspayload).

```c
#include "websockets.h"

void post(wsctx_t* ctx) {
    char* payload = websocketsrequest_payload(ctx->request->protocol);

    if (!payload) {
        ctx->response->send_text(ctx->response, "Payload not found");
        return;
    }

    ctx->response->send_text(ctx->response, payload);

    free(payload);
}
```

| Approach | Returns |
|----------|---------|
| `websocketsrequest_payload` / `protocol->get_payload` | `char*` — the whole body as a string |
| `websocketsrequest_payload_file` / `protocol->get_payload_file` | `file_content_t` — the body as a file descriptor |
| `websocketsrequest_payload_json` / `protocol->get_payload_json` | `json_doc_t*` — the body parsed as JSON |

## Connection

The `ctx->request->connection` field points to the underlying connection. It is used for broadcast channels — for example, to add a connection to a channel or to send a message to subscribers. See [Broadcasting](/en/wsbroadcast).
