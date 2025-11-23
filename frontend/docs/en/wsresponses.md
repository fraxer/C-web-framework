---
outline: deep
title: WebSocket responses
description: Sending WebSocket responses in C Web Framework. Text and binary messages, file sending, JSON responses.
---

# WebSocket responses

Forming and sending responses over a WebSocket connection. Text and binary data are supported.

## Sending a response

The content of the response is not sent to the user until the `text`, `binary`, or `file` methods are called. To send a response, you must call one of these methods at the end of the handler, otherwise the client will never wait for a response.

The WebSocket protocol allows you to send and receive data in text and binary formats.

Receiving and transmitting data must always be in the same format.

Text format

```c
void get(wsctx_t* ctx) {
    ctx->response->send_text(ctx->response, "{\"message\": \"This is json\"}");
}
```

Binary format

```c
void get(wsctx_t* ctx) {
    ctx->response->send_binary(ctx->response, "Text in binary format");
}
```

General format\
The `send_data` method automatically detects the format of the incoming request and generates a response in a similar format.

```c
void get(wsctx_t* ctx) {
    ctx->response->send_data(ctx, "Text in binary format");
}
```

## Sending files

The framework provides the `send_file` method for sending files over the WebSocket protocol.
The file is sent to the client in binary format.

```c
void get(wsctx_t* ctx) {
    ctx->response->send_file(ctx->response, "/path/image.jpg");
}
```

To be sure that no unwanted content is added to the response, when calling the `send_file` method, you do not need to call the `send_text` or `send_binary` method.
