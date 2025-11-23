---
outline: deep
description: Forming a server response, setting a status code, adding response headers, redirects, sending a file
---

# Responses

When the application finishes processing the request, it generates a response object and sends it to the user. The response object contains data such as the HTTP status code, HTTP headers, and the response body. The ultimate goal of developing a Web application is to create response objects for various requests.

In most cases, you will be dealing with the `response` application component, which is accessed via `ctx->response`.

## Status code

The first thing you do when building a response is determine if the request was successfully processed. This is done by setting the `status_code` property to a value that can be one of the valid HTTP status codes. For example, to indicate that a request was successfully processed, you can set the status code value to 200:

```C
void get(httpctx_t* ctx) {
    ctx->response->statusCode = 200;
    ctx->response->send_data(ctx->response, "Ok");
}
```

However, in most cases an explicit setting is not required, since the default value of `status_code` is 200. If you need to indicate that a request has failed, you can return a standard response:

```C
void get(httpctx_t* ctx) {
    int error = 1;
    if (error) {
        ctx->response->send_default(ctx->response, 500);
        return;
    }

    ctx->response->send_data(ctx->response, "Ok");
}
```

Full list of [HTTP codes](/http-codes)

## HTTP headers

You can send HTTP headers by working with the headers collection of the `response` component:

```C
void get(httpctx_t* ctx) {
    // add a Content-Type header. Content-Type headers already present will NOT be overwritten.
    ctx->response->add_header(ctx->response, 'Content-Type', 'text/plain');

    ctx->response->send_data(ctx->response, "Ok");
}
```

## Response body

Most responses should have a body containing what you want to show users.

If you already have a formatted string for the body, you can pass it to the `send_data` method of the `response` component

```C
void get(httpctx_t* ctx) {
    ctx->response->send_data(ctx->response, "Hello world!");
}
```

If your data needs to be formatted before being sent to end users, you should set the `Content-type` header.

```C
void get(httpctx_t* ctx) {
    ctx->response->add_header(ctx->response, 'Content-Type', 'application/json');

    ctx->response->send_data(ctx->response, "{\"message\": \"This is json\"}");
}
```

## Browser redirect

The browser redirect is based on sending the Location HTTP header.

You can redirect the user's browser to a URL by calling the `redirect` method. This method uses the specified URL as the value of the Location header and returns a response.

```C
void get(httpctx_t* ctx) {
    ctx->response->redirect(ctx->response, "/new-page", 301);
}
```

In code outside of action methods, you must manually set the `Location` header and call the `send_data` method immediately after it. So you can be sure that the answer will be correctly formed.

## Sending files

Like browser redirection, file upload is another feature based on specific HTTP headers. Cpdy provides a `send_file` method for handling file upload tasks.
The `send_file` method sends an existing file stream to the client as a file.

```C
void get(httpctx_t* ctx) {
    ctx->response->send_file(ctx->response, "/path/image.jpg");
}
```

To make sure that no unwanted content is added to the response, you do not need to call the `send_data` method when calling the `send_file` method.

## Sending a response

The content of the response is not sent to the user until the `send_data` or `send_file` methods are called. To send a response, you must call one of these methods at the end of the handler, otherwise the client will never wait for a response.
