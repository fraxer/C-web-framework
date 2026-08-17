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
- NewReno, CUBIC or BBR congestion control with pacing — selected with `http3_cc`
- Connection migration and path validation; issuing and retiring connection IDs; mid-connection key update (RFC 9001 §6); stateless reset
- A connection follows its datagrams: after a migration it is moved to whichever worker the kernel now delivers them to
- Client address validation: Retry token (`auto`/`always`/`never` policy) and `NEW_TOKEN` for returning clients; on hitting the connection limit the client gets `CONNECTION_REFUSED` instead of silence
- Anti-amplification protection (3×)
- IPv4 (IPv6 endpoints are not supported yet)

### HTTP/3

- Full frame set: DATA, HEADERS, SETTINGS, GOAWAY and more
- Control stream and QPACK encoder/decoder streams
- Request bodies (DATA, with tmp-file spilling for large bodies)
- **Trailers** and **103 Early Hints** — the same APIs (`add_trailer`, `add_early_hint`/`send_early_hints`) as in HTTP/2
- **100 Continue** — interim response
- **Concurrent requests** within one connection; the limit is `http3_max_streams_bidi` (default 100)

### Priorities (RFC 9218)

HTTP/2 deprecated its priority scheme and put nothing in its place; RFC 9218 is that replacement, and HTTP/3 implements it. The client states how urgent a response is, and the server sends in that order.

Two carriers, one meaning — an RFC 8941 dictionary with two members:

```http
GET /app.css HTTP/3
priority: u=0, i
```

| Member | Values | Default | Meaning |
|--------|--------|---------|---------|
| `u` | `0`–`7` | `3` | Urgency. `0` is the most urgent, `7` the least |
| `i` | boolean flag | absent | Incremental: the response is useful in pieces, so it may be interleaved with its peers |

The same value also arrives as a `PRIORITY_UPDATE` frame on the control stream — before the request stream exists or long after it did. The frame overrides the header field, but only for the members it carries: a `PRIORITY_UPDATE` saying just `u=5` does not reset an `i` the request established.

What the server does with it:

- **Different urgencies** — the more urgent response is sent first. A 4 KB file requested behind a 64 MB transfer arrives in 0.1 ms with `priority: u=0` instead of the 85 ms it waits without a signal.
- **Same urgency, not incremental** — responses are finished one at a time. What they block cannot start until they are done, so splitting the connection between them helps nobody.
- **Same urgency, incremental** — they share the connection, taking turns at the write budget whole rather than splitting it into slivers.

Nothing here needs configuring, and a connection that sends no priority signals takes the same path at the same cost as before. Whether the signals are reaching the scheduler is visible in `/metrics` → `http3.priority_applied`: "the client sends priorities" and "the server acts on them" are different claims, and this counter separates them.

::: tip Tolerance of malformed values
A value that is a well-formed dictionary but carries an unknown member, a member of the wrong type or an urgency outside 0–7 is **ignored** as RFC 9218 §4.1 requires — a cosmetic mistake by the peer must not cost a page. Only a value that is not a dictionary at all (a key with nothing after `=`, a stray comma) is an error, and only on the `PRIORITY_UPDATE` path, where the RFC makes it `H3_FRAME_ERROR`.
:::

The server does not send a `priority` header field in its responses — RFC 9218 §5 allows it to override its own urgency, but nothing consumes it.

### QPACK

QPACK is complete: dynamic tables on both sides, both instruction streams,
blocked request streams with acknowledgements and cancellation.

## Tuning

As with HTTP/2, the low-level parameters are environment variables from the `main.env` section. Every key is checked for type and range: a value of the wrong type or out of range is a configuration error, and the server refuses to start (a reload carrying such a value is rejected whole, leaving no mixture of old and new parameters).

### Transport and endpoint

| Parameter | Default | Description |
|-----------|---------|-------------|
| `http3_max_connections` | `65536` | Global process QUIC connection limit (64–4000000; `0` is invalid). When it is exhausted a new client gets `CONNECTION_REFUSED` |
| `http3_buffer_memory_limit` | `25% of RAM` | Process-wide budget for dynamic QUIC buffers (receive, send, CRYPTO), in bytes. `0` — disable the budget |
| `http3_rx_batch` | `32` | `recvmmsg` batch size (1–256) |
| `http3_so_rcvbuf` | `0` (kernel) | `SO_RCVBUF` for the UDP socket |
| `http3_so_sndbuf` | `0` | `SO_SNDBUF` for the UDP socket |
| `http3_handshake_rate` | `500` | New handshakes per second, per process. `0` — disable |
| `http3_handshake_burst` | `1000` | Handshake bucket peak |
| `http3_stateless_reset_rate` | `100` | Stateless reset bucket rate. `0` — disable |
| `http3_stateless_reset_burst` | `200` | Stateless reset bucket peak |
| `http3_version_negotiation_rate` | `100` | Version Negotiation reply rate. `0` — disable |
| `http3_version_negotiation_burst` | `200` | VN bucket peak |

`http3_max_connections` — how many QUIC connections the process holds at once, across all workers. The limit is about memory, not file descriptors: every connection carries crypto state, loss-recovery tables and buffers. Breaching it is not silent — the new client receives a `CONNECTION_CLOSE` with error `CONNECTION_REFUSED` and learns of the refusal immediately instead of timing out on its own. The current count, the peak and the limit are visible in `/metrics` → `quic.connections`; if `current` sits at the limit under working load, raise it after checking that memory allows.

`http3_buffer_memory_limit` — the process budget (in bytes) for everything that grows with load rather than with the connection count: datagram receive segments, send queues, handshake CRYPTO buffers, stream buffers and QPACK session memory. Exhausting it does not bring the server down: growth of a new buffer is simply refused, live connections keep running on what they already hold, and the `quic.memory.refused` counter in `/metrics` shows how often that happened. The default is a quarter of physical RAM, computed at startup; `0` disables the budget entirely (the server logs that). A growing `refused` under honest load is the signal that buffer memory is short.

`http3_rx_batch` — how many datagrams a single `recvmmsg` call fetches from the socket. Larger — fewer syscalls per packet under load; smaller — a shorter cycle on quiet traffic and less memory for the batch arrays. The default suits a typical server; change it from a profile, not a hunch.

`http3_so_rcvbuf` / `http3_so_sndbuf` — receive and send buffer sizes of the UDP socket (`SO_RCVBUF` / `SO_SNDBUF`), in bytes. `0` — leave the choice to the kernel. Raise the receive side when datagrams go missing in bursts: the kernel cannot drain its queue fast enough, which looks like unexplained loss on the server side. The kernel caps the maximum at `net.core.rmem_max` / `net.core.wmem_max` — you may ask for more, less will be installed.

`http3_handshake_rate` / `http3_handshake_burst` — a bucket on new handshakes per second, per process. The handshake is the most expensive part of a connection — key derivation and the server flight for every Initial, and an Initial is forged with a single `sendto`. The bucket refills at `rate` and holds `burst`, so an instantaneous spike of legitimate connects is not cut while a sustained flood is pinned at `rate`. When exhausted, the Initial is dropped silently — an honest client retransmits it on its own, and the `handshake_rate_limited` counter in `/metrics` shows the trip.

`http3_stateless_reset_rate` / `http3_stateless_reset_burst` — a stateless reset answers a packet addressed to a connection ID that no longer exists: the server was restarted, the connection timed out. The client needs that answer to stop waiting, but each one costs a key derivation, so spraying random CIDs must not buy computation from us — the bucket stops it.

`http3_version_negotiation_rate` / `http3_version_negotiation_burst` — Version Negotiation replies to a client offering an unknown QUIC version. The reply costs the sender nothing, so it is limited separately: this is the cleanest traffic-amplification candidate from a spoofed address.

### Address validation (Retry)

Retry solves a problem TCP does not have: in QUIC the client starts the handshake, and its address is spoofable with a single `sendto`. Until the address is proven, the server must treat the sender as untrusted — answer within the anti-amplification limit and spend the minimum on the handshake. Retry is an extra round trip on which the server challenges the client: a datagram from a spoofed address never sees the challenge and never answers it, while an honest client answers and proves it owns the address.

| Parameter | Default | Description |
|-----------|---------|-------------|
| `http3_retry` | `auto` | Retry policy: `auto` — engage past `http3_retry_threshold`, `always` — always, `never` — never |
| `http3_retry_threshold` | `1000` | Half-open handshake count at which `auto` starts answering with Retry (0–4000000) |
| `http3_new_token` | `true` | Issue a `NEW_TOKEN` after the handshake: the client's next connection proves its address without a Retry |
| `http3_token_lifetime_sec` | `86400` | Lifetime of `NEW_TOKEN` tokens, seconds (a Retry token lives a fixed 10 s) |

`http3_retry` — the policy of that proof. `auto` (default) engages Retry only under signs of attack, `always` — for every new client (maximum protection, at the price of a round trip for every new connection), `never` — never (a closed network or a test bench).

`http3_retry_threshold` — the threshold for `auto`: the number of handshakes started but not finished (`quic.handshakes.inflight` in `/metrics`). An honest client completes the handshake in tens of milliseconds and leaves this counter; what sticks in it are half-open handshakes from addresses that receive no replies — that is, a flood from spoofed addresses. Hence the rule: a thousand established connections is normal load, a thousand half-open ones is an attack. `0` turns `auto` into `always`.

`http3_new_token` — whether to issue a `NEW_TOKEN` after a completed handshake. The client stores it and presents it on its next connection: the address is already proven, no Retry is needed, and returning clients pay no round trip for address validation. The token key is generated fresh at every process start, so a server restart (or a load balancer without a shared key) voids previously issued tokens — for the client this is not an error, it simply goes through Retry once.

`http3_token_lifetime_sec` — how long a `NEW_TOKEN` is good for. RFC 9000 §8.1.3 requires bounding it so a stolen token cannot be replayed forever; the default is a day. The Retry token lives a fixed 10 seconds and is not governed by this key: it only needs to survive one round trip.

A Retry token that is ours but expired, or issued for another address, gets a loud `INVALID_TOKEN` (RFC 9000 §8.1.3), so a client looping on a token it cannot fix is never stuck. The `retry_sent`/`token_valid` pair in the `quic` section of `/metrics` answers "is Retry working": every Retry sent must come back with a valid token, and a persistent gap means clients are not getting through the extra round trip.

### Streams and flow control

| Parameter | Default | Description |
|-----------|---------|-------------|
| `http3_idle_timeout_sec` | `30` | Connection idle timeout, seconds (1–3600) |
| `http3_keepalive_sec` | `0` | Keep-alive PING interval, seconds (0–3600). `0` — do not hold silent connections open |
| `http3_max_udp_payload_size` | `1350` | Largest datagram the server sends and advertises (1200–1350) |
| `http3_initial_max_data` | `1048576` | Initial connection-level receive window (1 MiB) |
| `http3_initial_max_stream_data` | `262144` | Initial per-stream receive window (256 KiB) |
| `http3_max_streams_bidi` | `100` | How many request streams the client may open concurrently (1–65536) |
| `http3_max_streams_uni` | `8` | Unidirectional stream limit (3–65536; the floor of 3 is the protocol itself: control plus two QPACK) |
| `http3_recv_window_max` | `16777216` | Auto-tuning ceiling for the connection receive window, like `http2_recv_window_max` in HTTP/2 |
| `http3_active_cid_limit` | `4` | How many connection IDs to keep for the peer — the reserve for migration (2–8) |
| `http3_ack_delay_ms` | `25` | Maximum ACK delay the server advertises to the peer (0–16383, RFC 9000 §18.2) |

Flow control in QUIC works as in HTTP/2: the receiver advertises a window and grants more only as it consumes data. These parameters set the size of what the server promises to hold — a direct link to per-connection memory.

`http3_idle_timeout_sec` — the connection is closed after this many seconds without a packet from the client. The effective value is the minimum of ours and the one the client advertises: the smaller wins. A larger timeout lets mobile clients come back from sleep on the same connection, without a new handshake; a smaller one frees the memory of dead connections sooner. Active connections are not cut: the protocol keeps them alive on its own, and the timeout counts from the last packet.

`http3_keepalive_sec` — how often the server reminds the peer of itself with a PING frame so that a silent connection is not closed (RFC 9000 §10.1.2). Zero means it never does, and that is the default.

The key exists because `http3_idle_timeout_sec` only solves half the problem: the effective timeout is the **smaller** of the two advertised values, and browsers advertise about 30 seconds, so raising it on the server alone changes nothing. Without keep-alive, half a minute of pause costs the connection and the next navigation pays for a fresh handshake; with it, the connection survives pauses for as long as the browser answers.

The price of switching it on is that a connection lives — memory included — for as long as the client answers, which is why the default is off: how many connections to hold is decided by traffic, not by the protocol. A client that has gone away cannot be kept alive: only received packets count as activity, so a connection whose peer vanished still closes at the idle timeout however many PINGs were sent. The effective value is clamped to half the negotiated idle timeout (and to at least one second) — a PING has to be not only sent but acknowledged in time. How many went out is visible in `/metrics` as `quic.keepalive_sent`, kept apart from `quic.pto_probes_sent`: on the wire they are the same frame, but they mean opposite things — a keep-alive says nothing was happening, a probe says the path stopped answering.

`http3_max_udp_payload_size` — the largest datagram the server promises to accept: advertised to the client in transport parameters. It does not limit our outgoing datagrams — their size is picked by DPLPMTUD, from 1350 bytes up toward the path ceiling (1472 for IPv4, 1452 for IPv6), but never above what the client symmetrically promised. The floor of 1200 is the minimum RFC 9000 guarantees to traverse any path; the ceiling of 1350 is the buffer the server builds packets into — promising more would promise room the code does not have.

`http3_initial_max_data` — the initial receive window at the connection level: how many bytes the client may send, summed over all streams, before the server grants more window (`MAX_DATA`). This is a memory bound: exactly that much unread data may sit in the receive buffers at once.

`http3_initial_max_stream_data` — the same, per stream. It is the main brake on a single large upload: with a 256 KiB window, a client posting a gigabyte body stalls waiting for `MAX_STREAM_DATA` every 256 KiB. Auto-tuning takes over from there — the window grows when the server drains faster than a round trip.

`http3_max_streams_bidi` — how many request streams the client may open concurrently; the analogue of `SETTINGS_MAX_CONCURRENT_STREAMS` in HTTP/2. Every open stream is state and memory, hence the limit. A client opening a stream past the limit is misbehaving, and the connection closes with `STREAM_LIMIT_ERROR` — as the RFC requires. The default of 100 is what browsers are built around.

`http3_max_streams_uni` — the client's unidirectional stream limit. The protocol itself needs a floor of three: the HTTP/3 control stream and the two QPACK streams (encoder and decoder) — hence the minimum of 3. Today's clients need no more.

`http3_recv_window_max` — the ceiling the auto-tuner grows the connection receive window to. The rule is the same as in TCP: the window must hold the bandwidth-delay product, or speed is limited by the window rather than the path. For single large uploads over wide, long-latency paths, raise it. Setting it equal to `http3_initial_max_data` (it cannot go lower) pins the window without growth — like `http2_recv_window_max` in HTTP/2.

`http3_active_cid_limit` — how many connection IDs the server keeps ready for the client. A CID is the connection's future name on a new path: when the client changes networks (Wi-Fi → LTE, NAT rebind) it continues on the same connection, addressing it by a spare CID, and the address change does not break it. RFC mandates a minimum of 2; more is more migration headroom, at the cost of a few slots of memory.

Migration has a worker-side story too. There is nothing to configure there, but it is worth knowing when reading `/metrics`. The kernel hands datagrams to workers by hashing the address 4-tuple, while a QUIC connection outlives its address — so after a migration the packets arrive at a worker other than the one that accepted the connection. The server notices and moves the connection to it; `/metrics` → `quic.routing` reports `local`, `foreign` and `rehomed`. Healthy looks like `rehomed` in step with `migrations.validated` and a small `foreign`. A large `foreign` with `rehomed` at zero happens only during a reload, once the socket has been handed to the new generation.

`http3_ack_delay_ms` — the maximum acknowledgement delay the server advertises to the client. Instead of an ACK per received packet, the server may accumulate them and acknowledge several at once; the client subtracts the advertised value from its RTT estimate, so delayed ACKs do not inflate it. Larger — less ACK traffic on downloads; `0` — acknowledge every packet immediately.

### Congestion control

| Parameter | Default | Description |
|-----------|---------|-------------|
| `http3_initcwnd_packets` | `10` | Initial congestion window, in datagrams (2–64) |
| `http3_cc` | `newreno` | Congestion-control algorithm: `newreno`, `cubic` or `bbr` |
| `http3_pacing` | `true` | Spread sending over time instead of releasing the window at once (required by `http3_cc: "bbr"`) |
| `http3_amplification_factor` | `3` | How many times the server may answer before the address is proven (RFC 9000 §8.1, 1–16). Any departure from 3 is logged loudly at startup — change it only in tests |

`http3_initcwnd_packets` is the same choice TCP's `initcwnd` is. RFC 9002 §7.2 recommends ten datagrams and caps the initial window at 14 720 bytes — roughly 12 packets — which is not much on a long path: a 30 KB file then takes two round trips just to open the window, and at a 130 ms RTT that is another 130 ms on every asset. Anything other than 10 is a deliberate deviation, and the server says so at startup, in syslog:

```
quic: http3_initcwnd_packets is 30, not the 10 RFC 9002 §7.2 recommends
```

No such line means the value never reached the server: check that the key sits in `main.env`.

`http3_cc` is selected for each new connection; reloads affect new connections only. Values other than `newreno`, `cubic` and `bbr` reject the configuration.

All three answer the same question — how much data to keep in flight so the path is used fully without building a queue it cannot absorb. What differs is the evidence they answer it from, which is why the same three algorithms diverge by a factor of ten on some paths and not at all on others.

#### NewReno

The default, and the one RFC 9002 §7 spells out directly. The congestion window lives by four rules:

- **Slow start.** Every acknowledged byte adds a byte to the window, so the window doubles every round trip. It runs while the window is below the `ssthresh` threshold — which starts out infinite, meaning the first loss is what ends slow start.
- **Congestion avoidance.** From there the window grows by one datagram per window of acknowledged data — linear, one datagram per round trip. The remainder of that division is carried rather than dropped: without it growth would stall on large windows entirely, because an acknowledgement is almost always smaller than the window divided by the datagram size.
- **Loss.** The window and `ssthresh` are halved, but never below two datagrams. A recovery period starts at the same moment: everything sent before it began belongs to the same loss, so a burst of lost packets reduces the window **once** rather than once per packet.
- **Persistent congestion** (§7.6). When everything sent across a span longer than three PTOs is lost, this is not congestion but a path that stopped working: the window collapses to the minimum and slow start begins again. A PTO probe is separate from loss — it is allowed past the window, because eliciting an acknowledgement while the window is closed is precisely its job.

The weak spot is the price of a single loss. On a 100 Mbit/s path at 100 ms RTT roughly 900 datagrams fit in flight; one loss takes the window to 450, and it climbs back one datagram per round trip — on the order of 45 seconds at full speed. That hole is what the other two algorithms close, each in its own way.

#### CUBIC (RFC 9438)

CUBIC keeps the same model — a window reacting to loss — and changes both of Reno's constants, the decrease and the increase alike.

- **The decrease is gentler:** the window is multiplied by β = 0.7 rather than 0.5. The previous value is remembered as `W_max` — the point where the path already pushed back once.
- **Growth follows time, not rounds:** `W(t) = C·(t − K)³ + W_max`, with `C = 0.4` and `K = ∛(W_max·(1−β)/C)`, the time the curve needs to return to `W_max`. The shape of the cubic is the whole idea: right after the loss the window grows fast, it flattens out around `W_max` (that is where it hurt), and if nothing happens there either it accelerates again in search of a new ceiling. Independence from RTT is the second consequence: connections with different delays sharing a path get comparable shares, whereas under Reno a share is inversely proportional to RTT.
- **Fast convergence.** If the next loss arrives before the window has climbed back to the old `W_max`, the available bandwidth has shrunk — most likely a new neighbour showed up. `W_max` is then lowered to 0.85 of the current window, freeing room faster than the curve alone would.
- **The TCP-friendly region.** In parallel CUBIC computes the window Reno would have had at this point (α = 3(1−β)/(1+β) = 9/17 of a datagram per round trip) and takes the larger of the two. Without that rule the cubic curve would be slower than Reno on short RTTs and small windows — the "improved" algorithm losing on local paths.

The arithmetic is integer throughout, cube root included (binary search): the controller runs on every acknowledgement, and floating point on the transport hot path is a cost paid forever.

#### BBR (draft-cardwell-iccrg-bbr-congestion-control)

BBR answers a different question — not how much to keep in flight, but how fast to send. It still has a window, but only as the bound that keeps a mistaken rate from filling the path. Its model of the path is two measured quantities:

- **BtlBw** — the maximum delivery rate over a sliding window of 10 round trips. The rate is bytes delivered divided by the interval they took, and both ends of that interval are recorded when the packet is **sent** — otherwise the number measures the sender's own scheduling rather than the path. Samples marked `app-limited` (the data ran out before the window did) never lower the estimate: they measure the application, not the link.
- **RTprop** — the minimum RTT over the last 10 seconds, that is, the path's delay with no queue in it.

Sending runs at `BtlBw × gain` through the pacer, with the window held at `2 × BDP`. The gain comes from the current phase, and the phases are essentially the whole algorithm:

| Phase | What it does |
|-------|--------------|
| **STARTUP** | gain ≈ 2.89 (2/ln 2) — the rate doubles every round, the way slow start does. The pipe counts as full once three rounds in a row fail to raise the bandwidth estimate by 25 % |
| **DRAIN** | gain ≈ 0.35 — drain the queue STARTUP built, in about the time it took to build it |
| **PROBE_BW** | a cycle of eight rounds: one at 1.25× (is there more bandwidth), one at 0.75× (give back the queue that just created), six at the estimate itself. Which phase the cycle starts on is taken from the clock — otherwise connections that started together would probe in lockstep and measure their own convoy instead of the path |
| **PROBE_RTT** | at least once every 10 s the window drops to four datagrams for 200 ms: a standing queue hides the true propagation delay for exactly as long as it stands, so the only way to measure it is to empty the path |

Loss is not the model's signal, but it is not ignored either: the window comes down by exactly the bytes lost, and for one round packet conservation applies — only what leaves the flight goes back into it. A path that really is dropping traffic therefore stops receiving a full window while the model catches up. Persistent congestion resets the model outright and returns to STARTUP, keeping one thing only — RTprop: propagation delay is a property of the path, not of the congestion episode, and re-measuring it would cost a PROBE_RTT for nothing.

#### Comparison

| | NewReno | CUBIC | BBR |
|---|---|---|---|
| Decides | bytes in flight | bytes in flight | sending rate; the window is a bound |
| Signal | loss | loss | measured BtlBw and RTprop |
| Growth without loss | +1 datagram per RTT | cubic curve to `W_max` and beyond, RTT-independent | rate = BtlBw × phase gain |
| Reaction to loss | window ×0.5 | window ×0.7 plus fast convergence | −bytes lost, one round of packet conservation |
| Recovery from one loss at high BDP | tens of seconds | seconds | none needed: the model did not change |
| Loss that is not congestion (Wi-Fi, LTE) | collapses | holds up markedly better | barely notices |
| Relationship with the queue | fills the buffer until loss | fills the buffer until loss | holds ≈BDP, drains periodically |
| Sharing a narrow link | the most yielding | moderately more assertive than Reno | more assertive than both: sends faster than loss would permit |
| Pacing | preferred | preferred | **required** — it *is* the output |
| Per-connection state | window, threshold, recovery start | plus `W_max`, `K`, epoch start, minimum RTT | plus bandwidth filter, RTprop, phase, round counters |
| Periodic dips | none | none | PROBE_RTT: 4 datagrams for 200 ms every 10 s |
| Choose it for | paths where loss only ever means congestion; maximum politeness to neighbours | wired paths with a large bandwidth-delay product, long routes | paths where loss does not mean congestion: mobile, Wi-Fi, international routes |

A 64 MB transfer over loopback with injected loss (median of three runs) puts numbers on the difference:

| Loss | NewReno | CUBIC | BBR |
|------|---------|-------|-----|
| 0 % | 484 MB/s | 467 MB/s | 484 MB/s |
| 2 % | 329 MB/s | 387 MB/s | 387 MB/s |
| 10 % | 18 MB/s | 75 MB/s | **240 MB/s** |

On a clean path the choice does not matter; on a lossy one it decides
everything — 10 % loss turns NewReno into 18 MB/s while BBR stays at 240. The
trade-off is the one in the table: BBR deliberately sends faster than loss alone
would permit, so it is more assertive than CUBIC when sharing a narrow link. And
the brief dip once every ten seconds is PROBE_RTT, part of the algorithm rather
than a fault — on a monitoring graph it looks like a regular 200 ms notch in
throughput.

`bbr` requires `http3_pacing` to be on: the server drives its sending rate
through the pacer, and the pair `"bbr"` + `"http3_pacing": false` is rejected
when the configuration is loaded.

`http3_pacing` spreads sending instead of handing the window to the network in one piece. The burst budget is the **initial window**: the opening flight goes out whole, a raised `http3_initcwnd_packets` gets the burst it asked for, and what is spread is whatever the window frees later in the transfer — a single cumulative acknowledgement can free many times the initial window. Acknowledgements and PTO probes are never delayed. There is little reason to turn this off outside debugging.

`http3_amplification_factor` — how many times the server may answer before the address is proven. RFC 9000 §8.1 caps this at 3× the bytes received: an unlimited answer to an Initial would turn the server into a DDoS amplifier for a third party's address. Above three makes sense only on a test bench; any departure from 3 is logged loudly at startup — in production this value should not differ.

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

`http3_max_field_section_size` — the limit on a request's decoded header block. A soft breach yields `431 Request Header Fields Too Large` and the connection survives; the hard cap (×8 the limit) closes the connection with `H3_EXCESSIVE_LOAD`: a header block eight times the limit is not a request, it is an attack. The default megabyte covers long cookies and JWTs with room to spare; ordinary requests need a few kilobytes, and on a public server the limit is worth tightening.

`http3_abort_rate` / `http3_abort_burst` — the Rapid Reset budget (CVE-2023-44487): a client opens a stream and cancels it immediately, making the server do part of the work on each. Cancelling a request before the server has answered it spends from the bucket. Lone cancellations are normal behaviour — the user left the page — and the budget does not notice them; runs of them exhaust the bucket and close the connection with `H3_EXCESSIVE_LOAD`.

`http3_ctrl_rate` / `http3_ctrl_burst` — the limit on control frames that advance nothing: a `GOAWAY` or `MAX_PUSH_ID` repeating the current value, and frame types the server skips. A healthy connection carries a handful, so the limit is generous against the norm and closes the connection with `H3_EXCESSIVE_LOAD` only under an obvious flood.

`PRIORITY_UPDATE` is **not** covered by this limit and has no setting of its own. A browser sends one per request (Chrome sends two), so its rate is the request rate rather than a flood rate: any "frames per second" ceiling would sooner or later fall below honest traffic. What applies instead is a credit the server grants for each request it accepts — a peer that sends priorities without ever opening a stream spends the initial grant and gets `H3_EXCESSIVE_LOAD`, while an ordinary client never reaches the limit, because the number of requests it can make is already bounded by its QUIC stream credit. Exhaustion shows up in `/metrics` as `http3.abuse.priority_budget`.

Example:

```json
{
    "main": {
        "env": {
            "http3_max_connections": 50000,
            "http3_rx_batch": 64,
            "http3_cc": "bbr",
            "http3_max_field_section_size": 524288
        }
    }
}
```

## Limitations

| Feature | Status | Comment |
|-------------|--------|---------|
| **0-RTT / early data** | Yes, opt-in | `http3_early_data`, off by default; the request is not executed until the handshake completes |
| **Priorities (RFC 9218)** | Yes | The `priority` header field and `PRIORITY_UPDATE`, scheduled by urgency. The server does not send `priority` in its own responses |
| **Server Push** | No | Same rationale as in HTTP/2 |
| **WebSocket-over-h3** | No | Extended CONNECT (RFC 9220) is not planned: no browser supports it. WebSocket runs over HTTP/1.1 and HTTP/2 — clients open it over TCP |
| **HTTP/3 client** | No | Server role only |
| **CUBIC / BBR** | Yes | Both are selected with `http3_cc`; BBR requires `http3_pacing` |
| **UDP GSO** | Yes | Batched sends through `UDP_SEGMENT` |
| **GRO / ECN / DPLPMTUD** | Yes | GRO receive, validated ECN, and path-MTU probing with fallback |
| **IPv6 endpoint** | No | The QUIC endpoint binds IPv4 only. HTTP/1.1 and HTTP/2 over TCP are unaffected |
| **QUIC v2 (RFC 9369)** | No | A client offering an unknown version gets a Version Negotiation packet and comes back on v1, which is the correct answer; v2 itself exists mainly as an anti-ossification measure |
| **qlog** | No | The QUIC event log is not implemented — only a compile-time stub exists in the code |

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
