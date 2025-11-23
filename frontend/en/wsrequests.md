---
outline: deep
title: WebSockets requests
description: Data processing via the WebSockets protocol, description of the request format to the server
---

# WebSockets

Web sockets is an advanced technology that allows you to open a permanent bi-directional network connection between the user's browser and the server. With its API, you can send a message to the server and receive a response without making an http request, and this process will be event-driven.

## Requests

Requests made to an application are represented as a `wsctx_t` structure that provides information about the parameters of the WebSocket request - method, type, resource, data, and so on.

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

To get query parameters, you must call the `query()` method of the `websocketsrequest` component

```C
void get(wsctx_t* ctx) {
    const char* data = ctx->request->query(ctx->request, "mydata");

    if (data) {
        ctx->response->send_text(ctx->response, data);
        return;
    }

    ctx->response->send_text(ctx->response, "Data not found");
}
```

## POST and PATCH request data

The data that was sent via POST and PATCH is sent in the body of the request.

They are accessed through methods:

* payload()
* payload_json()

```C
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

You can get the name of the HTTP method used in the current request by accessing the `request->method` property.

The following methods are available:

* ROUTE_GET
* ROUTE_POST
* ROUTE_PATCH
* ROUTE_DELETE

```C
void get(wsctx_t* ctx) {
    const char* data = ctx->request->method == ROUTE_GET ?
        "This is the get method" :
        "This is the not get method";

    ctx->response->send_text(ctx->response, data);
}
```

## Request URL

The `request` component provides three properties for working with urls:

* uri - will return the relative address of the resource `/resource.html?id=100&text=hello`.
* path - will return the path to the resource without parameters `/resource.html`.
* ext - will return the extension from the resource name `html` or an empty string.

The length of each property is also calculated:

* uri_length
* path_length
* ext_length
