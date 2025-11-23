---
outline: deep
title: WebSockets responses
description: Sending a server response via the WebSockets protocol, transferring data in text or binary format, and an example of sending a file
---

# Responses

In most cases, you will be dealing with the `response` application component, which is a `wsctx_t` structure.

## Sending a response

The content of the response is not sent to the user until the `send_text`, `send_binary`, or `send_file` methods are called. To send a response, you must call one of these methods at the end of the handler, otherwise the client will never wait for a response.

The Websockets protocol allows you to send and receive data in text and binary formats.

Receiving and transmitting data must always be in the same format.

Text format

```C
void get(wsctx_t* ctx) {
    ctx->response->send_text(ctx->response, "{\"message\": \"This is json\"}");
}
```

Binary format

```C
void get(wsctx_t* ctx) {
    ctx->response->send_binary(ctx->response, "Text in binary format");
}
```

General format\
The `send_data` method automatically detects the format of the incoming request and generates a response in a similar format.

```C
void get(wsctx_t* ctx) {
    ctx->response->send_data(ctx->response, "Text in binary format");
}
```

## Sending files

Cpdy provides the `send_file` method for solving the problems of sending files using the WebSockets protocol.
The file is sent to the client in binary format.

```C
void get(wsctx_t* ctx) {
    ctx->response->send_file(ctx->response, "/path/image.jpg");
}
```

To be sure that no unwanted content is added to the response, when calling the `send_file` method, you do not need to call the `send_text` or `send_binary` method.
