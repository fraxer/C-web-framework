---
outline: deep
description: HTTP/2 in C Web Framework. Multiplexing, h2c upgrade, trailers, 103 Early Hints, WebSocket-over-h2, configuration and verification.
---

# HTTP/2

C Web Framework supports HTTP/2 (RFC 9113) — the second major version of HTTP with request multiplexing, binary framing, HPACK header compression and flow control.

::: tip In short
HTTP/2 is enabled **automatically** for any server with TLS configured — negotiated through ALPN. No separate flag or config section is required. Clients without h2 support transparently fall back to HTTP/1.1.
:::

## How it works

During the TLS handshake the server advertises `h2` and `http/1.1` through [ALPN](https://datatracker.ietf.org/doc/html/rfc7301), preferring `h2`. If the client also offers `h2`, the connection switches to HTTP/2 mode; otherwise it stays on HTTP/1.1. ALPN negotiation happens **after** the virtual host is selected via SNI, so HTTP/2 is available per server.

```json
{
    "servers": {
        "s1": {
            "domains": ["example.com"],
            "ip": "0.0.0.0",
            "port": 443,
            "tls": {
                "fullchain": "/etc/ssl/certs/fullchain.pem",
                "private": "/etc/ssl/private/privkey.pem",
                "ciphers": "TLS_AES_256_GCM_SHA384 TLS_CHACHA20_POLY1305_SHA256 ECDHE-RSA-AES256-GCM-SHA384"
            },
            "http": {
                "routes": { "/": { "GET": { "file": "...", "function": "index" } } }
            }
        }
    }
}
```

That is enough: once a `tls` section is present the server accepts h2 connections. Extra configuration is only needed to [tune behavior](#configuration) and for [h2c](#h2c-over-plaintext).

## h2c over plaintext

HTTP/2 without TLS (h2c) is supported in two modes and is opt-in — the server never switches to h2c on its own:

**1. Upgrade (RFC 9113 §3.2)** — an HTTP/1.1 request with `Upgrade: h2c` and `HTTP2-Settings` headers receives a `101 Switching Protocols` reply, after which the connection enters h2 mode. Implemented as a ready-made middleware `middleware_h2c_upgrade`:

```json
{
    "http": {
        "middlewares": ["middleware_h2c_upgrade"],
        "routes": { ... }
    }
}
```

```c
int middleware_h2c_upgrade(httpctx_t* ctx);
// returns 0 — if the request was upgraded (101 sent, chain stopped)
// returns 1 — for an ordinary request (continues to the handler)
```

**2. Prior-knowledge (RFC 9113 §3.4)** — the client sends the 24-byte magic preface `PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n` right away. The server detects this signature on a plaintext connection and opens an h2 session without an Upgrade handshake.

::: warning Plaintext only
h2c only makes sense for servers without TLS. For HTTPS servers the regular h2 over ALPN applies — the `h2c_upgrade` middleware is not needed there.
:::

## Features

### Multiplexing

Several concurrent requests over a single TCP connection. Each request is a separate stream; handlers of different streams run in parallel. The server advertises up to **100** concurrent streams per connection (`MAX_CONCURRENT_STREAMS`); excess streams receive `RST_STREAM`/`REFUSED_STREAM`.

### Flow control

Two-level flow control (connection + stream), default window 65535 bytes. The receive window auto-scales to the bandwidth-delay product (RTT is taken from `TCP_INFO`) up to `http2_recv_window_max`. This prevents large-body transfers from stalling on a narrow initial window.

### HPACK

Header compression through the static and dynamic HPACK tables with Huffman coding. Fields are validated strictly by tchar rules — stricter than the RFC letter, as in nghttp2.

### Trailers

Headers sent **after** the response body (e.g. `grpc-status` for gRPC), via a HEADERS frame with `END_STREAM`:

```c
int handler_with_trailers(httpctx_t* ctx) {
    httpresponse_t* res = ctx->response;

    res->add_header(res, "Content-Type", "application/grpc");
    send_data(ctx, "...payload...");

    /* The trailer is sent after the body as a separate HEADERS frame */
    res->add_trailer(res, "grpc-status", "0");
    res->add_trailer(res, "grpc-message", "OK");

    return 1;
}
```

::: warning HTTP/2 only
Trailers work only over h2. Over HTTP/1.1 `add_trailer()` returns 0 and logs a warning — chunked encoding with a `Trailer` header is not supported.
:::

### Early Hints (103)

An interim `103 Early Hints` response (RFC 8297) lets the browser start preloading resources while the handler is still preparing the final response. This is the recommended replacement for Server Push:

```c
int handler_with_hints(httpctx_t* ctx) {
    httpresponse_t* res = ctx->response;

    /* Hints are sent BEFORE the final response */
    res->add_early_hint(res, "Link", "</style.css>; rel=preload; as=style");
    res->add_early_hint(res, "Link", "</app.js>; rel=preload; as=script");
    res->send_early_hints(res);   /* → client receives 103 */

    /* ... handler works ... */

    send_data(ctx, "<!doctype html>...");   /* → final response */
    return 1;
}
```

`send_early_hints()` may be called several times; any call after the final response has begun is refused (1xx must precede the final response).

### WebSocket over HTTP/2

[Extended CONNECT](https://datatracker.ietf.org/doc/html/rfc8441) (RFC 8441) — a WebSocket session inside an h2 stream instead of a separate connection. The client initiates `:method: CONNECT` with `:protocol: websocket`. WebSocket handlers remain unchanged: the tunnel is transparent to application code, and broadcasting and `permessage-deflate` keep working.

For a vhost to accept such connections it must have a `websockets` section enabled — otherwise Extended CONNECT receives a `501 Not Implemented`.

## Configuration

HTTP/2 behavioral parameters are environment variables from the `main.env` section of `config.json`. They are read once at startup; the defaults are tuned for typical workloads.

### Lifecycle and flow

| Parameter | Default | Description |
|-----------|---------|-------------|
| `http2_idle_timeout_sec` | `120` | Connection idle timeout (sec). `0` — disable |
| `http2_ping_interval_sec` | `0` | Send PING after N sec of silence. `0` — watchdog off |
| `http2_ping_ack_timeout_sec` | `min(interval, 15)` | Grace for the PING ACK before closing the connection |
| `http2_settings_ack_timeout_sec` | `10` | SETTINGS ACK timeout (§6.5.3). `0` — disable |
| `http2_recv_window_initial` | `65535` | Initial receive window size |
| `http2_recv_window_max` | `4194304` | Auto-scaler ceiling (4 MB). Set equal to `initial` to disable scaling |
| `http2_write_quantum` | `65536` | Bytes a stream emits per round before yielding the socket (min 1024) |

### Abuse protection (DoS)

| Parameter | Default | Description |
|-----------|---------|-------------|
| `http2_max_header_list_size` | `32768` | Header block size limit. `0` — disable |
| `http2_max_continuation_frames` | `64` | CONTINUATION frames per block. `0` — no limit |
| `http2_abort_rate` | `100` | Rapid Reset budget (RST/sec, CVE-2023-44487). `0` — disable |
| `http2_abort_burst` | `200` | Peak size of the Rapid Reset bucket |

A soft breach of `http2_max_header_list_size` yields `431 Request Header Fields Too Large` (the connection survives); a hard breach (×8) yields `GOAWAY(ENHANCE_YOUR_CALM)`. Exhausting the Rapid Reset bucket also closes the connection via `GOAWAY`. Exact abuse counters are exposed under the `http2_abuse` section of the `/metrics` route.

Example configuration:

```json
{
    "main": {
        "env": {
            "http2_idle_timeout_sec": 60,
            "http2_ping_interval_sec": 30,
            "http2_max_header_list_size": 16384
        }
    }
}
```

## Limitations

| Feature | Status | Comment |
|---------|--------|---------|
| **Server Push** | Removed by decision | Implemented and passing `h2spec`, then deleted: Chrome removed push in 106, Firefox/Safari ship it off by default. Replacement — [103 Early Hints](#early-hints-103) |
| **Plain CONNECT** | Refused | Forbidden on purpose so the server does not become an open proxy. Do not confuse with Extended CONNECT for WebSocket |
| **HTTP/2 client** | None | Server role only. The framework's HTTP client runs over HTTP/1.1 |
| **Trailers / 103 over HTTP/1.1** | None | These capabilities exist only in h2 |
| **PRIORITY** | Ignored | `SETTINGS_NO_RFC7540_PRIORITIES = 1`; priorities are deprecated in RFC 9113 |

## Verification

### curl

```bash
# Force HTTP/2 (requires TLS)
curl -v --http2 https://example.com/

# Check ALPN negotiation
curl -v https://example.com/ 2>&1 | grep ALPN
# → * ALPN: server accepted h2.
```

### nghttp

```bash
# h2c via Upgrade on a plaintext server
nghttp -v http://example.com/

# h2 over TLS
nghttp -v https://example.com/
```

### Conformance

The implementation passes `h2spec` (147 tests, 0 failures over TLS):

```bash
h2spec -S -h example.com -p 443
```
