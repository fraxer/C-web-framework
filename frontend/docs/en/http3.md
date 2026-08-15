---
outline: deep
description: HTTP/3 in C Web Framework. QUIC transport, OpenSSL 3.5+ build, http3 config section, Alt-Svc, coexistence with HTTP/1.1 and HTTP/2.
---

# HTTP/3

C Web Framework supports HTTP/3 (RFC 9114) — the third major version of HTTP over QUIC (RFC 9000). QUIC runs over UDP and removes the pain points of HOL-blocking, TCP slow start and connection-setup latency: the handshake combines transport and TLS 1.3, and every request runs as an independent stream.

::: tip In short
HTTP/3 is enabled **on request**: it requires the `-DINCLUDE_HTTP3=yes` build flag and an `http3` section in the server configuration. Once enabled it is advertised to clients automatically through the `Alt-Svc` header over HTTP/1.1 and HTTP/2.
:::

## Requirements

| Component | Requirement |
|-----------|------------|
| **OpenSSL** | **3.5.0** or higher — for the QUIC TLS API (`SSL_set_quic_tls_cbs` and friends) |
| **Network** | A reachable UDP port (defaults to the server's TCP port) |
| **TLS** | A `tls` section is mandatory — QUIC has no cleartext mode (no h3c analogue) |

The framework uses **only** the QUIC TLS API from libssl. The entire QUIC transport stack (framing, loss recovery, congestion control, CID demultiplexer) is hand-written and does not rely on OpenSSL's built-in QUIC.

## Build

HTTP/3 is off by default. Enable it with a CMake flag:

```bash
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DINCLUDE_POSTGRESQL=yes \
         -DINCLUDE_MYSQL=yes \
         -DINCLUDE_REDIS=yes \
         -DINCLUDE_SQLITE=yes \
         -DINCLUDE_HTTP3=yes          # ← HTTP/3 / QUIC
cmake --build . -j$(nproc)
```

The `-DINCLUDE_HTTP3=yes` flag verifies that OpenSSL is ≥ 3.5 and that the built libssl actually exports the QUIC TLS API (some distros ship `no-quic` builds at the same version number). The rest of the framework stays compatible with OpenSSL 1.1.1+, so HTTP/3 is the only component with the higher requirement.

::: warning Building without the flag
If you enable `http3` in `config.json` but build the server without `-DINCLUDE_HTTP3=yes`, the server reports a configuration error and refuses to start.
:::

## Configuration

HTTP/3 is configured by an `http3` section inside a specific server. The same vhost keeps serving HTTP/1.1 and HTTP/2 over TCP — the UDP port serves only h3.

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
                "ciphers": "TLS_AES_256_GCM_SHA384 TLS_CHACHA20_POLY1305_SHA256"
            },
            "http": {
                "routes": { "/": { "GET": { "file": "...", "function": "index" } } }
            },
            "http3": {
                "enabled": true,
                "port": 443,
                "alt_svc": true,
                "alt_svc_max_age": 86400
            }
        }
    }
}
```

### Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `enabled` | bool | `false` | Enables HTTP/3 for the server. Required for h3 to run |
| `port` | number | server's TCP port | UDP port for QUIC (1–65535). Defaults to the vhost's `port` — what Alt-Svc advertises and what clients try first |
| `alt_svc` | bool | `true` | Advertise HTTP/3 in the `Alt-Svc` header over HTTP/1.1 and HTTP/2 responses |
| `alt_svc_max_age` | number | `86400` | How long the client caches `Alt-Svc` (sec) |

::: tip Why no host in Alt-Svc
The `Alt-Svc` value is built as `h3=":port"; ma=max_age` — with no host name. Per RFC 7838 an empty host means "the same one", which is correct for every vhost on this listener and keeps clients from pinning to a name the certificate may not cover.
:::

With `http3.enabled: true` and no `tls` section this is a configuration error ("http3 requires a tls section"); QUIC mandates TLS 1.3.

## Coexistence with HTTP/1.1 and HTTP/2

A single vhost serves all three protocol versions at once:

```
TCP :443  →  ALPN negotiates  h2  or  http/1.1
UDP :443  →  only  h3
```

- ALPN negotiation is split: a QUIC connection offers **only** `h3`, TCP offers `h2` and `http/1.1`. Mixing is forbidden.
- Vhost selection inside a connection is by **SNI**, identical to TCP+TLS.
- The client learns about h3 availability from the `Alt-Svc` header that the server adds to HTTP/1.1 and HTTP/2 responses. A browser never probes UDP speculatively — it learns about h3 from this header (or from a DNS HTTPS record).

After receiving `Alt-Svc: h3=":443"; ma=86400` the browser tries HTTP/3 in the background and switches on success. If UDP is blocked the client silently stays on TCP — no server-side fallback logic is needed.

## Features

### QUIC transport

- The handshake combines transport and TLS 1.3 — a single RTT to establish
- NewReno or CUBIC congestion control with pacing (BBR planned)
- Connection migration and path validation; Retry token for address validation; stateless reset
- Anti-amplification protection (3×)
- IPv4 (IPv6 endpoints are not supported yet)

### HTTP/3

- Full frame set: DATA, HEADERS, SETTINGS, GOAWAY and more
- Control stream and QPACK encoder/decoder streams
- Request bodies (DATA, with tmp-file spilling for large bodies)
- **Trailers** and **103 Early Hints** — the same APIs (`add_trailer`, `add_early_hint`/`send_early_hints`) as in HTTP/2
- **100 Continue** — interim response
- **Concurrent requests** within one connection (verified: 8 simultaneous requests)

### QPACK

QPACK is complete: dynamic tables on both sides, both instruction streams,
blocked request streams with acknowledgements and cancellation.

## Configuration

As with HTTP/2, the low-level parameters are environment variables from the `main.env` section.

### Transport and endpoint

| Parameter | Default | Description |
|-----------|---------|-------------|
| `http3_max_connections` | `65536` | Global process QUIC connection limit (64–4000000; `0` is invalid) |
| `http3_rx_batch` | `32` | `recvmmsg` batch size (1–256) |
| `http3_so_rcvbuf` | `0` (kernel) | `SO_RCVBUF` for the UDP socket |
| `http3_so_sndbuf` | `0` | `SO_SNDBUF` for the UDP socket |
| `http3_stateless_reset_rate` | — | Stateless reset bucket rate. `0` — disable |
| `http3_stateless_reset_burst` | — | Stateless reset bucket peak |
| `http3_version_negotiation_rate` | — | Version Negotiation reply rate. `0` — disable |
| `http3_version_negotiation_burst` | — | VN bucket peak |

### Congestion control

| Parameter | Default | Description |
|-----------|---------|-------------|
| `http3_initcwnd_packets` | `10` | Initial congestion window, in datagrams (2–64) |
| `http3_cc` | `newreno` | Congestion-control algorithm: `newreno` or `cubic` |
| `http3_pacing` | `true` | Spread sending over time instead of releasing the window at once |

`http3_initcwnd_packets` is the same choice TCP's `initcwnd` is. RFC 9002 §7.2 recommends ten datagrams and caps the initial window at 14 720 bytes — roughly 12 packets — which is not much on a long path: a 30 KB file then takes two round trips just to open the window, and at a 130 ms RTT that is another 130 ms on every asset. Anything other than 10 is a deliberate deviation, and the server says so at startup, in syslog:

```
quic: http3_initcwnd_packets is 30, not the 10 RFC 9002 §7.2 recommends
```

No such line means the value never reached the server: check that the key sits in `main.env`.

`http3_cc` is selected for each new connection. `newreno` is the more
conservative default. `cubic` implements RFC 9438: it reduces the window to
70% after a loss and then recovers along the cubic curve, while its
TCP-friendly region keeps it competitive with Reno on small windows. It is
typically useful on paths with a large bandwidth-delay product. Reloads affect
new connections only. Values other than `newreno` and `cubic` reject the
configuration.

`http3_pacing` spreads sending instead of handing the window to the network in one piece. The burst budget is the **initial window**: the opening flight goes out whole, a raised `http3_initcwnd_packets` gets the burst it asked for, and what is spread is whatever the window frees later in the transfer — a single cumulative acknowledgement can free many times the initial window. Acknowledgements and PTO probes are never delayed. There is little reason to turn this off outside debugging.

### 0-RTT (early data)

| Setting | Default | Description |
|----------|--------------|----------|
| `http3_early_data` | `false` | Accept 0-RTT: a resuming client's request arrives a round trip earlier |

A client that has been here before and kept a session ticket can send its
request with the very first packet, without waiting for the handshake. That
saves a full round trip: on a 130 ms path the page starts loading 130 ms sooner.

The price is built into the protocol: **a 0-RTT request is replayable**. Anyone
who copied the datagram can send it again, and the server cannot tell the copy
from the original — AEAD proves authenticity, not freshness.

This server answers that by **not executing the request until the handshake
completes**. The data is accepted into its streams, but no handler runs: no
database write, no session lookup. A copy cannot complete the handshake — the
attacker holds no key material — so a replayed request does nothing and the
connection dies at the idle timeout. Two practical consequences:

- restricting 0-RTT to safe methods (GET/HEAD) is **not required** — a POST in
  early data is as safe as one in an ordinary connection;
- the response is not sent before the handshake ends. What is saved is the trip
  **to** the server, not back.

Turning it on is the operator's decision: it is off by default.

```json
{ "main": { "env": { "http3_early_data": true } } }
```

Worth knowing in operation: tickets are bound to the transport parameters they
were issued under. Change `http3_initial_max_data`, `http3_max_streams_bidi`,
`http3_idle_timeout_sec` or any other setting from the tables above, and
previously issued tickets stop resuming — clients do one full handshake. That is
RFC 9001 §7.4.1 at work, not a failure.

To confirm it is doing something, read the `quic` section of `/metrics`:

```
"early_data.offered": 128,
"early_data.accepted": 126,
"early_data.packets": 141,
"early_data.bytes": 13904
```

`offered` minus `accepted` is refused tickets: either the configuration changed,
or the replay defence fired.

### Abuse protection

| Parameter | Default | Description |
|-----------|---------|-------------|
| `http3_max_field_section_size` | `1048576` | Header block size limit (1 MB) |
| `http3_abort_rate` | `100` | Rapid Reset budget. `0` — disable |
| `http3_abort_burst` | `200` | Rapid Reset bucket peak |
| `http3_ctrl_rate` | `100` | Control-frame flood limit (GOAWAY, etc.). `0` — disable |
| `http3_ctrl_burst` | `200` | Control bucket peak |

Example:

```json
{
    "main": {
        "env": {
            "http3_max_connections": 50000,
            "http3_rx_batch": 64,
            "http3_cc": "cubic",
            "http3_max_field_section_size": 524288
        }
    }
}
```

## Limitations

| Feature | Status | Comment |
|-------------|--------|---------|
| **0-RTT / early data** | Yes, opt-in | `http3_early_data`, off by default; the request is not executed until the handshake completes |
| **Server Push** | No | Same rationale as in HTTP/2 |
| **WebSocket-over-h3** | No | Extended CONNECT (RFC 9220) is not planned: no browser supports it. WebSocket runs over HTTP/1.1 and HTTP/2 — clients open it over TCP |
| **HTTP/3 client** | No | Server role only |
| **CUBIC / BBR** | Partial | Select CUBIC with `http3_cc`; BBR is not supported yet |
| **UDP GSO** | Yes | Batched sends through `UDP_SEGMENT` |
| **GRO / ECN / DPLPMTUD** | Yes | GRO receive, validated ECN, and path-MTU probing with fallback |
| **qlog** | Yes | QUIC diagnostic logging |

## Verification

### curl

```bash
# Requires a curl built with HTTP/3 support
curl -v --http3 https://example.com/

# Check the Alt-Svc header over TCP
curl -sI https://example.com/ | grep -i alt-svc
# → alt-svc: h3=":443"; ma=86400
```

### Browser

Chrome switches to HTTP/3 automatically after receiving the `Alt-Svc` header. In DevTools on the **Network** tab, the **Protocol** column shows `h3` for QUIC requests.

### openssl

```bash
# Verify the built libssl exports the QUIC TLS API
openssl version
# → OpenSSL 3.5.x (or higher)
```
