---
outline: deep
title: WebSocket responses
description: Sending WebSocket responses in C Web Framework. Text and binary frames, file sending, PONG and CLOSE control frames, automatic permessage-deflate compression.
---

# WebSocket responses

Forming and sending frames back to the client over a WebSocket connection. The response is accessible through the `wsctx_t->response` object (`websocketsresponse_t*`). Response methods are function pointers attached to the object, so the first argument is always the object itself.

```c
#include "websockets.h"

void my_handler(wsctx_t* ctx) {
    ctx->response->send_text(ctx->response, "Hello World");
}
```

The WebSocket protocol carries data in frames of two payload types — **text** (opcode `0x81`) and **binary** (opcode `0x82`). The framework provides methods for each format, plus a generic one that picks the format from the incoming request.

## When the response is sent

A frame is prepared and queued for sending only when one of the `send_*` methods is called. Until then, nothing goes to the wire — so at the end of a handler you must call `send_text` / `send_binary` / `send_data` / `send_file` (or a control-frame function). If no method is called, the client receives no reply for that request.

Once the response is sent, the handler should return — sending twice within a single request produces an invalid frame.

## Methods overview

| Method | Arguments | Description |
|--------|-----------|-------------|
| `send_text` | `(response, data)` | Text frame, null-terminated string |
| `send_textn` | `(response, data, length)` | Text frame with explicit length |
| `send_binary` | `(response, data)` | Binary frame, length via `strlen` |
| `send_binaryn` | `(response, data, length)` | Binary frame with explicit length |
| `send_data` | `(ctx, data)` | Frame in the request's format (text/binary) |
| `send_datan` | `(ctx, data, length)` | Same, with explicit length |
| `send_file` | `(response, path)` | File as a binary frame, path relative to server root |
| `send_filen` | `(response, path, length)` | Same, with explicit path length |

::: tip The first argument differs
Most methods take the response object — `ctx->response`. The exception is `send_data` / `send_datan`: they need the whole context `ctx` to read the incoming request type. That is why the call looks like `ctx->response->send_data(ctx, ...)`.
:::

## Text messages

`send_text` sends a null-terminated string as a text frame. If the data contains embedded null bytes or its length is known up front, use the `send_textn` variant with an explicit length:

```c
void get(wsctx_t* ctx) {
    ctx->response->send_text(ctx->response, "{\"message\": \"This is json\"}");
}

void getn(wsctx_t* ctx) {
    const char data[] = { 'H', 'i', 0x00, '!' };
    ctx->response->send_textn(ctx->response, data, sizeof(data));
}
```

## Binary messages

`send_binary` sends a binary frame. Note that this variant determines the length with `strlen()`, so it is **unsafe for data with embedded null bytes** — everything after the first null is lost. For real binary content use `send_binaryn` with an explicit length:

```c
// Safe for text without nulls
void get(wsctx_t* ctx) {
    ctx->response->send_binary(ctx->response, "Text in binary format");
}

// Safe for arbitrary bytes
void raw(wsctx_t* ctx) {
    const char data[] = { 0x00, 0x01, 0x02, 0x03, 0x00, 0xFF };
    ctx->response->send_binaryn(ctx->response, data, sizeof(data));
}
```

## Automatic format

The `send_data` method inspects the incoming request type and sends the reply in the same format: a text frame if the request was text, otherwise binary. It is handy for echo handlers and any case where the reply format must match the request format.

```c
void echo(wsctx_t* ctx) {
    ctx->response->send_data(ctx, "echo");          // null-terminated
    ctx->response->send_datan(ctx, "echo", 4);      // with explicit length
}
```

::: tip One format per connection
WebSocket does not require the request and response to share a format, but keeping them consistent is easier in practice — the client parser then always knows what to expect. `send_data` removes the need to check the request type manually.
:::

## Sending files

`send_file` reads a file and sends it as a single binary frame. The path is relative to the server root (`server.root` from `config.json`); the `send_filen` variant takes an explicit path length:

```c
void get(wsctx_t* ctx) {
    ctx->response->send_file(ctx->response, "files/image.jpg");
}
```

The method returns `0` on success and `-1` on error. An error case is **not left without a reply**: when the file is missing or is a directory, a text message is sent automatically (`"resource not found"` / `"resource forbidden"`). There is no need to call `send_text` or `send_binary` alongside `send_file` — doing so would only corrupt the frame.

```c
void get(wsctx_t* ctx) {
    int result = ctx->response->send_file(ctx->response, "files/report.pdf");

    if (result == -1) {
        // File missing or inaccessible — an error text frame has already been sent.
        return;
    }
}
```

## Control frames

Besides text and binary, the protocol defines control frames. They are sent through free functions (not object methods) declared in `websocketsresponse.h`:

| Function | Opcode | Purpose |
|----------|--------|---------|
| `websocketsresponse_pong(response, data, length)` | `0x8A` | Reply to an incoming `PING`; usually echoes the ping payload |
| `websocketsresponse_close(response, data, length)` | `0x88` | Initiate connection close; `data` is an optional reason/code |

```c
void close_handler(wsctx_t* ctx) {
    // Close the connection with code 1000 (Normal Closure)
    const char reason[] = { 0x03, 0xE8 }; // 1000, big-endian
    websocketsresponse_close(ctx->response, reason, sizeof(reason));
}
```

::: warning Control frames are a low-level API
Normally the framework answers `PING` with a `PONG` on its own and negotiates closing. `websocketsresponse_pong` / `websocketsresponse_close` are only needed for manual control over the connection lifecycle.
:::

## Compression (permessage-deflate)

If the connection negotiated the `permessage-deflate` extension (RFC 7692), the framework **automatically** compresses outgoing text and binary frames of 128 bytes or larger. The compressed frame sets the RSV1 bit, and the payload length is encoded as usual.

Compression is applied only when it actually reduces the size; otherwise the frame is sent uncompressed. If compression fails, the framework falls back to an uncompressed send too — there is nothing for the handler to check.

Configuring extension negotiation is described under the WebSocket route settings (`config.json`).

## Notes

- Nothing is sent to the client until one of `send_*` (`send_text`, `send_textn`, `send_binary`, `send_binaryn`, `send_data`, `send_datan`, `send_file`, `send_filen`) or a control function (`websocketsresponse_pong` / `websocketsresponse_close`) is called.
- After sending the response, return from the handler — sending twice within a single request produces an invalid frame.
- `send_binary` computes the length with `strlen()` — for binary data with embedded nulls use `send_binaryn`.
- `send_file`/`send_filen` send the file relative to the server root and produce an error frame on access failure on their own, so no extra send is needed.
- Reading data from the client is covered in [Receiving data from the client](/en/wspayload); fanning messages out to subscribers — in [Broadcasting](/en/wsbroadcast).
