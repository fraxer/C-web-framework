---
outline: deep
description: Processing http requests, extracting cookies and request parameters, processing payloads, working with headers
---

# Requests

Requests made to an application are represented as a `httpctx_t` structure that provides information about the HTTP request parameters - headers, cookie, method, etc.

## GET request parameters

To get query parameters, you must call the `query()` method of the `request` component

```C
void get(httpctx_t* ctx) {
    const char* data = ctx->request->query(ctx->request, "mydata");

    if (data) {
        ctx->response->send_data(ctx->response, data);
        return;
    }

    ctx->response->send_data(ctx->response, "data not found");
}
```

## Parameters of POST, PUT, PATCH requests

Unlike GET parameters, parameters that were passed via POST, PUT, PATCH, etc. sent in the body of the request.

They are accessed through methods:

* get_payloadf()
* get_payload_filef()
* get_payload_jsonf()

```C
void post(httpctx_t* ctx) {
    char* payload = ctx->request->get_payloadf(ctx->request, "mydata");

    if (!payload) {
        ctx->response->send_data(ctx->response, "field not found");
        return;
    }

    ctx->response->send_data(ctx->response, payload);

    free(payload);
}
```

Processing of incoming data is described in detail in the section [Receiving data from the client](/payload)

## Request methods

You can get the name of the HTTP method used in the current request by accessing the `ctx->request->method` property.

The following methods are available:

* ROUTE_GET
* ROUTE_POST
* ROUTE_PUT
* ROUTE_DELETE
* ROUTE_OPTIONS
* ROUTE_PATCH

```C
void get(httpctx_t* ctx) {
    const char* data = ctx->request->method == ROUTE_GET ?
        "This is the get method" :
        "This is the not get method";

    ctx->response->send_data(ctx->response, data);
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

```C
void get(httpctx_t* ctx) {
    if (ctx->request->uri_length > 4096) {
        ctx->response->send_default(ctx->response, 414);
        return;
    }

    ctx->response->send_data(ctx->response, ctx->request->uri);
}
```

## HTTP headers

You can get information about HTTP headers using the methods

`ctx->request->get_header()` or `ctx->request->get_headern()`. For example,

```C
void get(httpctx_t* ctx) {
    const http_header_t* header_host = ctx->request->get_header(ctx->request, "Host");
    // size_t key_length = 4;
    // const http_header_t* header_host = ctx->request->get_headern(ctx->request, "Host", key_length);

    if (header_host) {
        ctx->response->send_data(ctx->response, header_host->value);
        return;
    }

    ctx->response->send_data(ctx->response, "header Host not found");
}
```
