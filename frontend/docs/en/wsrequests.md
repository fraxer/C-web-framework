---
outline: deep
title: WebSocket requests
description: Processing WebSocket requests in C Web Framework. Message format, parameter extraction, GET/POST/PATCH/DELETE methods.
---

# WebSocket requests

WebSocket provides a persistent bidirectional connection between the client and server, allowing real-time data exchange without HTTP overhead. With its API, you can send a message to the server and receive a response without making an HTTP request, and this process will be event-driven.

## Requests

Requests made to an application are represented as a `websocketsrequest_t` structure that provides information about the WebSocket request parameters - method, type, resource, data, and so on.

Request format: `[method] [path] [payload]`.

The method, path, and payload are separated by a single space, for example:

```
GET /
GET /users?id=100
POST /users {"id": 100, "name":"Alex"}
PATCH /users/100 {"name":"Alen"}
DELETE /users/100
```

## GET request parameters

To get query parameters, use the `wsctx_t` context:

```c
#include "query.h"

void get(wsctx_t* ctx) {
    int ok = 0;
    const char* data = query_param_char(ctx->request, "mydata", &ok);

    if (ok) {
        ctx->response->send_text(ctx->response, data);
        return;
    }

    ctx->response->send_text(ctx->response, "Data not found");
}
```

## POST and PATCH request data

The data that was sent via POST and PATCH is sent in the body of the request.

They are accessed through functions:

* `websocketsrequest_payload(protocol)`
* `websocketsrequest_payload_json(protocol)`

```c
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

## Request methods

You can get the name of the HTTP method used in the current request through the protocol.

The following methods are available:

* ROUTE_GET
* ROUTE_POST
* ROUTE_PATCH
* ROUTE_DELETE

```c
void get(wsctx_t* ctx) {
    const char* data = ctx->request->method == ROUTE_GET ?
        "This is the get method" :
        "This is the not get method";

    ctx->response->send_text(ctx->response, data);
}
```

## Request URL

The `request` component provides three properties for working with URLs:

* uri - will return the relative address of the resource `/resource.html?id=100&text=hello`.
* path - will return the path to the resource without parameters `/resource.html`.
* ext - will return the extension from the resource name `html` or an empty string.

The length of each property is also calculated:

* uri_length
* path_length
* ext_length
