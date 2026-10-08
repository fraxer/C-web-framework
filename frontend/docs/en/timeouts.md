---
outline: deep
description: HTTP and WebSocket timeouts in C Web Framework. Configuration, inheritance, Ping/Pong, slow request monitoring and logs.
---

# Timeouts and slow requests

The server measures request reception, queueing, handler execution and response sending. Timeouts bound network stages; `slow_request_threshold_ms` exposes slow requests, including queue and handler delays, before the response completes.

Checks cover HTTP/1.0, HTTP/1.1, HTTP/2 and HTTP/3. WebSocket uses HTTP/1.1 Upgrade and the existing HTTP/2 extended CONNECT path. WebSocket over HTTP/3 is not supported yet.

## Configuration

Add `timeouts` to the existing `main` section. This is a configuration fragment; retain the other required settings.

```json
{
    "main": {
        "timeouts": {
            "request_header_timeout_ms": 15000,
            "request_body_idle_timeout_ms": 30000,
            "request_body_total_timeout_ms": 300000,
            "slow_request_threshold_ms": 1000
        }
    }
}
```

Every expired timeout terminates what it bounds: the request, the stream or the connection. There is no observe-only mode. The defaults apply without a `timeouts` section too, so every server bounds header reception, body pauses, sending and WebSocket liveness out of the box. The slow request threshold only records an event and does not cancel a handler.

HTTP/2 and HTTP/3 use the same fields. The former `main.env.http2_request_timeout_sec` was removed; a configuration that still sets it is rejected, so its HTTP/2 budgets cannot change silently. Other HTTP/2 and QUIC transport limits have their own settings.

Durations are integer milliseconds in `0..86400000`. Zero disables a check, except for these, which must be positive: header and body idle, `slow_request_threshold_ms`, `ws_close_timeout_ms`, and the Pong timeout when Ping is enabled. See [Fields](#fields) for every field and default.

## Fields

`main.timeouts` sets the common policy; `servers.<name>.timeouts` overrides individual fields for a virtual host. Missing fields inherit.

| Field | Default, ms | Meaning |
| --- | --- | --- |
| `request_header_timeout_ms` | 60000 | Absolute HTTP header deadline; HTTP/1.x also bounds first-byte/h2c preface waiting |
| `request_body_idle_timeout_ms` | 60000 | No body progress on this request |
| `request_body_total_timeout_ms` | 0 | Total body reception budget |
| `slow_request_threshold_ms` | 1000 | Slow request event threshold |
| `tls_handshake_timeout_ms` | 10000 | TCP TLS handshake; QUIC handshake uses the HTTP/3 transport policy |
| `keepalive_timeout_ms` | 75000 | Waiting for the next HTTP/1.x request |
| `response_send_idle_timeout_ms` | 60000 | No HTTP response send progress |
| `ws_handshake_timeout_ms` | 10000 | WebSocket establishment after HTTP headers |
| `ws_ping_interval_ms` | 30000 | WebSocket heartbeat interval |
| `ws_pong_timeout_ms` | 10000 | Matching Pong deadline after Ping has actually been sent |
| `ws_message_idle_timeout_ms` | 60000 | No progress in an incomplete message/frame |
| `ws_message_total_timeout_ms` | 0 | Whole message budget including fragments |
| `ws_send_idle_timeout_ms` | 60000 | Stalled ready output or queued heartbeat |
| `ws_close_timeout_ms` | 5000 | Closing budget |
| `ws_application_idle_timeout_ms` | 0 | No completed application messages; Pong does not extend this |

HTTP body deadlines stop applying once a WebSocket session is established; a quiet session with matching Pong remains open when application idle is disabled.

## Headers, body and sending

`request_header_timeout_ms` is an absolute header reception budget. Individual bytes do not extend it. HTTP/1.x also bounds waiting for the first bytes and an incomplete h2c preface.

`request_body_idle_timeout_ms` limits a pause without body reception progress. For example, a 30-second idle budget does not expire when new body data arrives every 20 seconds. `request_body_total_timeout_ms` bounds the whole upload starting at header completion and does not reset when bytes arrive. Its default is disabled (`0`).

`response_send_idle_timeout_ms` limits lack of progress sending a ready response. Handler execution before a response becomes ready is a separate stage. With `Expect: 100-continue`, stalled Continue output is checked as sending; an incomplete Continue does not count as the client's upload pause.

`tls_handshake_timeout_ms` bounds TCP TLS handshakes, and `keepalive_timeout_ms` bounds waiting for the next HTTP/1.x request on a free connection. QUIC TLS and HTTP/2/QUIC connection idle have transport settings described on the [HTTP/2](/en/http2#configuration) and [HTTP/3](/en/http3) pages.

| Situation | Action |
| --- | --- |
| Incomplete HTTP/1.x request | 408 and connection closure, when response ordering permits |
| HTTP/1.x connection without a request byte (fresh or keep-alive) | Silent closure, as in nginx: a pooled client could take a 408 for the reply to the request it is sending |
| HTTP/1.x response in flight or a pipeline queue | Connection closure; a 408 is not inserted ahead of earlier responses |
| HTTP/2 body reception | `RST_STREAM(CANCEL)` for the affected stream |
| Incomplete HTTP/2 framing or HEADERS/CONTINUATION | Connection termination with GOAWAY: shared parser state prevents stream isolation |
| HTTP/3 reception or sending | Affected stream cancellation using `STOP_SENDING`/`RESET_STREAM` with `H3_REQUEST_CANCELLED` |
| HTTP/1.x response sending or TCP TLS handshake | Connection closure |

Other HTTP/2/3 streams do not extend this request's body idle clock. Checks use monotonic time. Workers check periodically (TCP tick: 500 ms) and on input paths; actual termination depends on event loop scheduling and lock availability. A 100 ms setting does not guarantee closure at exactly 100 ms.

## Inheritance and routes

Precedence: built-in defaults → `main.timeouts` → `servers.<name>.timeouts` → permitted method route settings. Partial objects replace only listed fields. For example, for a large upload:

```json
{
    "servers": {
        "s1": {
            "timeouts": {
                "request_body_idle_timeout_ms": 60000
            },
            "http": {
                "routes": {
                    "/upload": {
                        "POST": {
                            "file": "handlers/libapp.so",
                            "function": "upload",
                            "timeouts": {
                                "request_body_total_timeout_ms": 600000
                            }
                        }
                    }
                }
            }
        }
    }
}
```

HTTP routes allow only `request_body_idle_timeout_ms`, `request_body_total_timeout_ms` and `slow_request_threshold_ms`. Virtual hosts sharing a TCP listener must agree on header/TLS budgets, so policy is unambiguous before Host/SNI is parsed.

An unknown key, invalid type, fraction, forbidden zero or disallowed override rejects loading with the setting's path. Policies are checked at startup and before switching generations on [reload](/en/hot-reload). New connections receive the new policy; the old generation retains its own until it ends under the usual soft/hard reload rules. An invalid candidate leaves the serving generation active.

## WebSocket

In `main.timeouts` or `servers.<name>.timeouts`, configure heartbeat and message reception:

```json
{
    "timeouts": {
        "ws_handshake_timeout_ms": 10000,
        "ws_ping_interval_ms": 30000,
        "ws_pong_timeout_ms": 10000,
        "ws_message_idle_timeout_ms": 60000,
        "ws_message_total_timeout_ms": 300000,
        "ws_send_idle_timeout_ms": 60000,
        "ws_close_timeout_ms": 5000,
        "ws_application_idle_timeout_ms": 0
    }
}
```

Only one server Ping is outstanding; a matching Pong must repeat its payload. An unrelated Pong, incoming Ping or application message does not acknowledge heartbeat. The Pong deadline starts after Ping is actually sent; a queued Ping is covered by send idle. `ws_ping_interval_ms: 0` disables server heartbeat. HTTP/2 PING does not replace WebSocket Ping/Pong.

Message idle covers a started message or incomplete frame, including its header. Total bounds one message across fragmentation; interleaved Ping/Pong do not extend it. Message idle does not apply between messages. A quiet session with matching Pong remains open with `ws_application_idle_timeout_ms: 0`. A positive application idle budget requires completed application messages; Pong does not reset it.

On timeout, the server attempts Close with code `1008` in both HTTP/1.1 and HTTP/2 (H2 previously used `1000` for policy timeout). `ws_close_timeout_ms` bounds closing. An incomplete input frame or stalled output may cause immediate transport termination. HTTP 408 is never sent after Upgrade. On HTTP/2, terminating the session cancels its stream.

`websockets.routes.<path>.<method>.timeouts` permits all `ws_*` fields except `ws_handshake_timeout_ms`. A resource route is selected by a message, so its settings become active after the first complete message selecting that route. Each subsequent route selection inherits afresh from the virtual host. `websockets.default` has no separate route override. See [WebSocket requests](/en/wsrequests) for route structure.

HTTP/2 has one absolute header deadline from the first HEADERS byte through the entire CONTINUATION block. Moving to another frame does not extend it. Before frame classification, an incomplete preface or transport frame header retains the existing connection receive budget; SETTINGS ACK and idle/PING checks remain active.

A classified DATA payload does not use the HTTP header timeout. Useful body fragments extend body idle before the full frame is assembled; padding does not. A body timeout cancels that stream, then drains and discards the remaining DATA to the next frame boundary. TCP cannot deliver the neighboring frame until that remainder arrives. Body total never extends. An operation expires exactly at its deadline: late bytes before the next tick cannot bypass the deadline.

HTTP/2 HEADERS/CONTINUATION receive time is included in `headers_ms`, `total_ms` and the access log. A live `slow_request phase=headers` event is available before END_HEADERS and is transferred to the request without duplication. Trailers do not change the request start or the completion time of its initial headers.

A fully sent WebSocket Ping clears pending send state: with an empty output queue, only the Pong deadline applies. Another probe is allowed after the previous one is acknowledged. A Pong at or after the deadline cannot revive the session. Read-path and tick timeouts initiate the same closure.

## Logs

Enable info-level logging for timeout events:

```json
{
    "main": {
        "log": { "enabled": true, "level": "info" }
    }
}
```

```text
slow_request protocol=h2 phase=body age_ms=1000
request_timeout protocol=h2 reason=body_idle fd=12 stream=1
websocket_timeout protocol=websocket reason=pong fd=12 stream=0
slow_request_complete headers_ms=2 body_ms=1500 queue_ms=1 handler_ms=5 send_ms=2 total_ms=1510
```

These are example records; fields depend on the event and protocol. `slow_request` marks threshold crossing once per request accessible to the worker and identifies its current phase: `headers`, `body`, `queue`, `handler` or `send`. `slow_request_complete` reports durations at response completion when the total threshold is exceeded. Cancellation of incomplete requests is recorded as `request_timeout`; final duration records are not guaranteed for every cancellation.

Events are deduplicated by reason: a slow event cannot suppress a timeout. A new HTTP request, heartbeat or send episode can record a new expiration. Timeout records share `protocol`, `reason`, `fd` and `stream`; HTTP/1.x uses `stream=0`.

The final [access log](/en/config#access) field is seconds since the first request bytes, including reception, queueing, handler execution and sending. For HTTP/3, completion is handing the response to QUIC; a separate send idle check monitors transport sending. The lifetime of an open WebSocket session is not part of the HTTP establishment request duration.

## Implementation limits

Closing a connection or cancelling a stream does not stop a running C handler function. There is no general handler deadline, cancel token or cooperative database call cancellation here. Minimum upload rate is also not checked: regular chunks can extend idle, so use a total budget to bound the entire upload.

Timeouts do not replace [body size limits and storage settings](/en/config#client-max-body-size).
