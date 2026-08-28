---
outline: deep
description: Complete configuration reference for C Web Framework. Every section and key of config.json — main, env, servers, routes, ratelimits, TLS, HTTP/3, databases, storages, sessions, task_manager, translations, mail, mimetypes.
---

# Configuration file

The whole application is configured from a single JSON file; there are no separate `.env` files. The path is passed with `-c`:

```bash
./exec/cwfr -c /path/to/config.json      # daemonises (Release)
./exec/cwfr -c /path/to/config.json -f   # stay in the foreground
```

`-f` is what you want under a supervisor or in a container: a process that forks and exits reads to systemd or docker as one that died immediately.

Reload the configuration without stopping the server with `SIGUSR1` (`pkill -USR1 cwfr`); the behaviour is set by [`main.reload`](#reload). See [Hot reload](/en/hot-reload) for details.

## How the configuration is read

The file is parsed in full **before** the server listens on any socket, and any error stops start-up with a message in the journal (syslog, `journalctl -t cwfr`). The order is `main` → `servers` → `databases` → `storages` → `mimetypes` → `sessions` → `task_manager` → `translations` → `mail`, then the HTTP/2 and HTTP/3 policies from [`main.env`](#env).

Two practical consequences:

* **Handler paths are checked at start-up.** The `.so` files named in `routes` and `task_manager` are loaded immediately and the function names resolved through `dlsym`. A typo in a path or a function name means the server does not come up.
* **The exit code means something.** The process does not report success until the config has been read, validated and applied, **and every worker is listening**. So `cwfr -c config.json && ...` does what it looks like it does.

### Which sections are required

| Section | Required | What happens without it |
|---------|----------|-------------------------|
| [`main`](#main-section) | **yes** | Start-up fails |
| [`servers`](#servers-section) | **yes**, non-empty | Start-up fails |
| [`mimetypes`](#mimetypes-section) | **yes**, non-empty | Start-up fails |
| [`databases`](#databases-section) | no | No database access |
| [`storages`](#storages-section) | no | No file storages |
| [`sessions`](#sessions-section) | no | No sessions |
| [`task_manager`](#task-manager-section) | no | The scheduler does not start |
| [`translations`](#translations-section) | no | i18n disabled |
| [`mail`](#mail-section) | no | DKIM fields empty, mail goes unsigned |
| [`migrations`](#migrations-section) | no | Nothing: the section is not read at runtime |

## main section

Required. **Every key below except `env` is mandatory** — a missing one stops start-up.

### workers <Badge type="info" text="number"/> <Badge type="danger" text="required"/>

How many workers accept connections and read/write data. Must be ≥ 1.

Workers are **threads of one process**, not separate processes. Anything documented as "per process" (the QUIC memory budget, the compressed-representation cache, the QUIC connection limit) therefore is *not* multiplied by their number.

### threads <Badge type="info" text="number"/> <Badge type="danger" text="required"/>

How many threads run handlers and build responses. Must be ≥ 1. Kept separate from the workers on purpose: a slow handler must not stop the sockets from being read.

### reload <Badge type="info" text="soft | hard"/> <Badge type="danger" text="required"/>

Hot-reload mode (on `SIGUSR1`):

* `soft` — reload keeping active connections
* `hard` — reload forcibly closing connections

The code default is `soft`, but the key still has to be present in the file.

### client_max_body_size <Badge type="info" text="number"/> <Badge type="danger" text="required"/>

Maximum request body size in bytes, ≥ 1.

It is checked twice, with different outcomes: a `Content-Length` header above the limit is a `400 Bad Request` while the headers are still being parsed, and the body is never read; a body that grows past the limit while being received (`chunked`, HTTP/2, HTTP/3) is a `413 Content Too Large`. The same value caps WebSocket frames and the responses the built-in HTTP client accepts.

### tmp <Badge type="info" text="string"/> <Badge type="danger" text="required"/>

Temporary file directory: large request bodies and uploads are spooled there. **No trailing slash** — `"/tmp/"` is a configuration error, `"/tmp"` is correct.

### gzip <Badge type="info" text="array of strings"/> <Badge type="danger" text="required"/>

MIME types eligible for automatic response compression. The key is mandatory, but the array may be empty (`[]`) — that turns compression off.

A type on this list becomes **negotiable**, not always compressed. A response is compressed only if it is at least 1 KB and the request's `Accept-Encoding` allows it: `gzip` named explicitly, or `*` when gzip is not mentioned; `gzip;q=0` is a refusal, and a request with no `Accept-Encoding` at all gets uncompressed bytes. Every response whose type is on this list carries `Vary: Accept-Encoding` — compressed or not — so a shared cache tells the two representations apart; their `ETag`s differ too (the compressed one gets a `-gzip` suffix).

Compressing static files is expensive and paid again on every request: a 92 KB file costs about 410 µs of CPU in zlib — four times everything else the response does. Two keys in [`main.env`](#env) remove that work, each in its own way; both are off by default.

#### gzip_static <Badge type="info" text="boolean"/> <Badge type="tip" text="main.env"/>

Serve a ready-made `<file>.gz` when one sits next to the file: the server then compresses nothing at all — the body comes off disk with `Content-Length` instead of `chunked`.

The twin is used only if it is **not older** than the original — a stale build artefact is never served, on-the-fly compression takes over instead. The check costs one `open()` and runs only for responses that would have been compressed anyway (type on the `main.gzip` list, client accepts gzip, size at least 1 KB), so a client that did not ask for compression does not pay for it.

```json
"env": { "gzip_static": true }
```

The server **does not create** these files — it only serves the ones already there, and writing them is the build's job. If your build cannot, post-process the output directory:

```bash
find dist -type f \( -name '*.html' -o -name '*.css' -o -name '*.js' \) -size +1k \
    -exec gzip -9 -k -f {} +
```

Only the types listed in `main.gzip` are worth compressing: no twin is looked for next to a file of any other type. The `-size +1k` threshold mirrors the server's — a file under 1 KB is not compressed and its `.gz` is never asked for.

Regenerate **after every build**. `gzip -k` preserves the original's `mtime`, so a fresh twin passes the staleness check; a twin left over from the previous build will be older than the new files, and the server quietly falls back to on-the-fly compression without saying so in the log.

`Content-Length` in such a response is the length of the **compressed** bytes: HTTP counts the body after the content-coding is applied, and `chunked` is unnecessary because the size is known before the first byte goes out (with on-the-fly compression it is not, which is why that path uses `chunked`). The uncompressed size is not reported in any header, but it does go into the `ETag` — the validators are taken from the original before the `.gz` is substituted.

#### gzip_cache_size, gzip_cache_max_file <Badge type="info" text="numbers"/> <Badge type="tip" text="main.env"/>

An in-memory cache of compressed representations: a file is compressed once, and every later request gets the finished bytes. Useful where you cannot place `.gz` files next to the originals.

| Key | Default | Description |
|-----|---------|-------------|
| `gzip_cache_size` | `0` (off) | Total memory budget for compressed representations, in bytes |
| `gzip_cache_max_file` | `1048576` | Largest source file that will be cached, in bytes |

```json
"env": { "gzip_cache_size": 33554432, "gzip_cache_max_file": 1048576 }
```

The entry key is the path, the `mtime` **and** the size of the source file, so an overwritten file is never served from stale content: change any of the three and it is a different resource, compressed afresh. The budget is kept by least-recently-used eviction, and `gzip_cache_max_file` stops one large file from evicting everything else; an entry being read by an unfinished response survives until that response ends, even if the cache has already released it. A ceiling larger than the budget is clamped to the budget — an entry that size could never fit anyway.

The cache is one per server: workers are threads of a single process, so `gzip_cache_size` is not multiplied by their number.

The `ETag` does not depend on the mode: a compressed representation gets a weak one (`W/"…-gzip"`) describing the resource rather than the exact bytes, so a cache hit, a `.gz` from disk and on-the-fly compression are interchangeable to a client cache. The order is: `.gz` from disk first, then the in-memory cache, then on-the-fly compression.

### log <Badge type="info" text="object"/> <Badge type="danger" text="required"/>

Logging settings. Both nested fields are mandatory. The journal goes to **syslog** — read it with `journalctl -t cwfr`, not from stdout.

```json
"log": {
    "enabled": true,
    "level": "info"
}
```

#### enabled <Badge type="info" text="boolean"/>

Enables or disables logging. With `false` all logging functions are ignored.

#### level <Badge type="info" text="string"/>

Minimum log level. Messages of lower priority are filtered out. Accepted values (most to least critical):

- `emerg` — system is unusable (0)
- `alert` — action must be taken immediately (1)
- `crit` — critical condition (2)
- `err` / `error` — errors (3)
- `warning` / `warn` — warnings (4)
- `notice` — normal but significant (5)
- `info` — informational (6)
- `debug` — debug messages (7)

Anything else is a configuration error.

::: tip Recommendations
- **Production:** `info` or `notice` — a balance between detail and performance.
- **Development:** `debug` — maximum detail.
- **Critical systems:** `warning`/`error` — important events only.
:::

### env <Badge type="info" text="object"/>

The only optional key in `main`. A key-value store holding **both** your application's own settings **and** every behavioural parameter of the protocols.

Only scalar values are copied: `string`, `number`, `bool`, `null`. Nested objects and arrays are **silently dropped** — no `env_get_*` function can read them.

#### The .env file

Keys can also come from a `.env` file placed next to `config.json` — one `key=value` pair per line:

```dotenv
# a comment
refresh_token_expiration=15552000
feature_x_enabled=true
app_name=backend
password="p@ss # not a comment"  # a comment after the value
```

Blank lines and lines starting with `#` are skipped, an `export ` prefix is allowed, whitespace around keys and values is trimmed. Matching quotes are stripped, and a quoted value always stays a string (for an unquoted value, everything after ` #` is dropped as a comment). The type is inferred: `true`/`false` reads as a boolean, anything that parses as a number reads as a number, everything else is a string — from there a key is indistinguishable from one set in `main.env`.

The path is overridden with [`env_file`](#env-file). If a key is set both in `main.env` and in `.env`, `main.env` wins.

#### env_file <Badge type="info" text="string"/> <Badge type="tip" text="main"/>

Name of the variables file instead of `.env`. A relative path is resolved against the directory of `config.json`:

```json
"env_file": "secrets/.env.production"
```

If the file cannot be opened, or a line in it has no `=`, a message goes to the journal and startup continues without those keys.

#### Application keys

```json
"env": {
    "refresh_token_expiration": 15552000,
    "feature_x_enabled": true,
    "app_name": "backend"
}
```

```c
long long ttl = env_get_llong("refresh_token_expiration", 3600);
int enabled  = env_get_bool("feature_x_enabled", 0);
const char* name = env_get_string("app_name", "default");
```

Available: `env_get_string`, `env_get_int`, `env_get_llong`, `env_get_bool`, `env_get_double`, `env_get_ldouble` — each takes a key and a default. A missing key, or one of the wrong type, yields the default without an error.

When a call-site default is not expressive enough — for example, a missing key must be distinguishable from an explicitly set value — use the **checked** variants: `env_get_string_checked`, `env_get_llong_checked`, `env_get_bool_checked`. They return a status: `0` — the key is absent, `1` — the key exists with a suitable type (the value is written to the out parameter), `-1` — the key exists but has the wrong type. The standalone `env_config_get_string_checked` / `env_config_get_llong_checked` / `env_config_get_bool_checked` do the same for an explicitly passed `env_t*` rather than the global configuration — handy in code that works with a configuration before it is published, and in tests.

#### Runtime keys

Everything else configured through `main.env` is a server parameter. A full index; each parameter is described in detail on the linked page.

| Group | Keys | Documented in |
|-------|------|---------------|
| Observability | `metrics` | [below](#metrics) |
| Static compression | `gzip_static`, `gzip_cache_size`, `gzip_cache_max_file` | [above](#gzip-static) |
| HTTP/2: lifecycle | `http2_idle_timeout_sec`, `http2_ping_interval_sec`, `http2_ping_ack_timeout_sec`, `http2_settings_ack_timeout_sec`, `http2_recv_window_initial`, `http2_recv_window_max`, `http2_write_quantum` | [HTTP/2 → Configuration](/en/http2#configuration) |
| HTTP/2: abuse protection | `http2_max_header_list_size`, `http2_max_continuation_frames`, `http2_abort_rate`, `http2_abort_burst`, `http2_ctrl_rate`, `http2_ctrl_burst`, `http2_max_out_backlog` | [HTTP/2 → Abuse protection](/en/http2#abuse-protection-dos) |
| HTTP/3: endpoint and resources | `http3_max_connections`, `http3_buffer_memory_limit`, `http3_rx_batch`, `http3_so_rcvbuf`, `http3_so_sndbuf`, `http3_handshake_rate`, `http3_handshake_burst`, `http3_stateless_reset_rate`, `http3_stateless_reset_burst`, `http3_version_negotiation_rate`, `http3_version_negotiation_burst` | [HTTP/3](/en/http3) |
| HTTP/3: connection transport | `http3_idle_timeout_sec`, `http3_keepalive_sec`, `http3_max_udp_payload_size`, `http3_initial_max_data`, `http3_initial_max_stream_data`, `http3_max_streams_bidi`, `http3_max_streams_uni`, `http3_recv_window_max`, `http3_active_cid_limit`, `http3_ack_delay_ms` | [HTTP/3](/en/http3) |
| HTTP/3: congestion | `http3_initcwnd_packets`, `http3_cc`, `http3_pacing`, `http3_amplification_factor` | [HTTP/3](/en/http3) |
| HTTP/3: address validation | `http3_retry`, `http3_retry_threshold`, `http3_new_token`, `http3_token_lifetime_sec` | [HTTP/3](/en/http3) |
| HTTP/3: protocol and abuse | `http3_max_field_section_size`, `http3_abort_rate`, `http3_abort_burst`, `http3_ctrl_rate`, `http3_ctrl_burst` | [HTTP/3](/en/http3) |
| HTTP/3: versions and 0-RTT | `http3_version_2`, `http3_early_data` | [HTTP/3](/en/http3) |
| HTTP/3: diagnostics | `http3_qlog_dir`, `http3_qlog_connections` | [HTTP/3](/en/http3) |

All of them are read once when the configuration is loaded and re-read on reload. **A wrong type or an out-of-range value is a configuration error, not a silent fallback to the default:** `"http3_cc": "reno"` or `"http3_max_streams_uni": 1` stop start-up with a message naming the key and the accepted range.

#### metrics <Badge type="info" text="boolean"/>

Enables the concurrency, lock, HTTP/2 and QUIC counters. `false` by default: counters that are off cost nothing.

```json
"env": { "metrics": true }
```

The flag only collects the data — exposing it is an application handler's job. In the example application that is `app/routes/bench/metrics.c`, mounted on a route:

```json
"/metrics": {
    "GET": { "file": "<build>/exec/handlers/bench/lib_metrics.so", "function": "metrics" }
}
```

`GET /metrics?reset=1` returns a snapshot and zeroes the counters. The route is worth putting behind a middleware — the snapshot describes the process's internal state.

### Keys that do not exist

`main.buffer_size` appears in older `config.json` examples and is **not read by the server**: the connection buffer size is fixed in code. The key is harmless but changes nothing — it can be deleted.

## migrations section

```json
"migrations": {
    "source_directory": "<project>/app/migrations"
}
```

::: warning This section is unused
Neither `cwfr` nor `migrate` reads `migrations.source_directory`. The `migrate` utility takes its paths as positional arguments:

```bash
./exec/migrate create add_users_table ../config.json ../app/migrations/s1
./exec/migrate up all ../config.json postgresql.p1 s1
```

Keep the section as project documentation or delete it — behaviour is the same either way. See [Migrations](/en/migrations).
:::

## translations section

A list of localisation (i18n) domains. Optional.

```json
"translations": [
    { "domain": "backend", "path": "/app/locale" }
]
```

* `domain` — text domain name (required, non-empty)
* `path` — directory holding the `.mo` files (required, non-empty)

Each domain's base locale is `en`. An entry with invalid fields is **skipped with a message in the journal** and the rest are loaded: unlike most sections, a broken entry here does not stop start-up. See [Internationalisation](/en/i18n).

## task_manager section

Background tasks loaded from `.so` files and run by the scheduler. Optional; an array.

The common fields of every task — `name`, `type`, `file`, `function` — are mandatory; the schedule fields depend on `type`.

| `type` | Schedule fields | Description |
|--------|-----------------|-------------|
| `interval` | `interval` | Every N seconds (≥ 1) |
| `daily` | `hour`, `minute` | Daily at `hh:mm` |
| `weekly` | `weekday`, `hour`, `minute` | Weekly on the given weekday |
| `monthly` | `day`, `hour`, `minute` | Monthly on the given day |

`weekday` accepts `sunday`, `monday`, `tuesday`, `wednesday`, `thursday`, `friday`, `saturday`; `hour` is 0–23, `minute` 0–59, `day` 1–31. An unknown `type` or an out-of-range value is a configuration error.

```json
"task_manager": [
    {
        "name": "cleanup_expired_tokens",
        "type": "interval",
        "interval": 60,
        "file": "/app/build/exec/handlers/tasks/libtasks.so",
        "function": "cleanup_authorization_codes"
    },
    {
        "name": "nightly_report",
        "type": "daily",
        "hour": 3,
        "minute": 30,
        "file": "/app/build/exec/handlers/tasks/libtasks.so",
        "function": "send_report"
    },
    {
        "name": "weekly_digest",
        "type": "weekly",
        "weekday": "monday",
        "hour": 9,
        "minute": 0,
        "file": "/app/build/exec/handlers/tasks/libtasks.so",
        "function": "send_digest"
    },
    {
        "name": "monthly_invoice",
        "type": "monthly",
        "day": 1,
        "hour": 0,
        "minute": 15,
        "file": "/app/build/exec/handlers/tasks/libtasks.so",
        "function": "build_invoices"
    }
]
```

See [Task manager](/en/task-manager).

## servers section

Required and non-empty. Each server (virtual host) is a named entry; the name (`s1`, `s2`, …) is arbitrary and used nowhere except error messages.

**Required vhost fields:** `domains`, `ip`, `port`, `root`. The rest — `index`, `ratelimits`, `http`, `websockets`, `tls`, `http3` — are optional.

### domains <Badge type="info" text="array of strings"/> <Badge type="danger" text="required"/>

The names this vhost answers to. Matching is against the `Host` header (or the SNI name on TLS), **case-insensitively** and **after the port is stripped**.

The template is converted to punycode, then into a regular expression by a few rules:

| In the template | Means |
|-----------------|-------|
| `example.com` | An exact name. The dot is escaped, so `a.b` does not match `axb` |
| `*.example.com` | Outside brackets `*` expands to `.*` — but **only at the start or the end of the string** |
| `mail.*` | The same from the other end |
| `(api\|www).example.com` | Ordinary regular expression, PCRE2: groups, alternation and `[...]` classes behave as in a regex |
| `(.1\|.*)example.com` | Inside brackets the dot and the asterisk are **metacharacters**; no escaping is applied |

The template is anchored automatically: `^` and `$` are added if absent. An asterisk in the middle (`a*b`) is a configuration error, as is an unbalanced bracket.

A template with no metacharacters is compared directly, bypassing PCRE2 — noticeably faster, and precisely why the dot is escaped rather than left as a metacharacter: otherwise no real domain name would ever take the fast path.

::: warning Do not put a port in domains
The port is stripped from `Host` before matching, so an entry like `"www.example.com:8080"` will **never** match. The port is set by the [`port`](#port) field.
:::

A vhost is identified by the triple **(address, domain, port)**, and two entries sharing that triple are a configuration error.

### ip <Badge type="info" text="string"/> <Badge type="danger" text="required"/>

The address to listen on — IPv4 or IPv6. `127.0.0.1`, `0.0.0.0`, `::1`, `::` and the bracketed form `[::1]` are all accepted. The address applies both to TCP (HTTP/1.1, HTTP/2, WebSocket) and to the HTTP/3 UDP endpoint.

An IPv6 socket is created **v6-only**: it does not accept IPv4 traffic, because a dual-stack socket would hand back IPv4 addresses in v4-mapped form, which makes a datagram's local address ambiguous and depends on the system's `net.ipv6.bindv6only`. A site reachable over both families is therefore **two entries** in `servers` with the same `domains` and `port`, differing only in `ip`:

```json
"servers": {
    "site_v4": { "domains": ["example.com"], "ip": "0.0.0.0", "port": 443, "...": "..." },
    "site_v6": { "domains": ["example.com"], "ip": "::",      "port": 443, "...": "..." }
}
```

Vhost uniqueness is checked on "address + domain + port", so such a pair is not a collision.

An invalid address (a typo like `127.0.0.300` or `::1x`) **stops start-up** with a message naming the value — the server does not try to guess it.

### port <Badge type="info" text="number"/> <Badge type="danger" text="required"/>

The server's TCP port (usually `80` for HTTP, `443` for HTTPS). It also becomes the default UDP port for HTTP/3.

### root <Badge type="info" text="string"/> <Badge type="danger" text="required"/>

The static file root. It **must exist at start-up**, otherwise the server refuses to start. A trailing slash is stripped.

Everything file-related is resolved from this directory: both the static files served when no route matches and the paths in [`static_file`](#routes).

### index <Badge type="info" text="string"/>

The index file name served for a directory. Defaults to `index.html`. One name, not a list.

### ratelimits <Badge type="info" text="object"/>

Named rate-limiting profiles (token bucket). Each profile sets `burst` (bucket capacity — the peak number of requests) and `rate` (tokens refilled per second); the window is one second.

```json
"ratelimits": {
    "one":   { "burst": 1,  "rate": 0  },
    "strict":{ "burst": 15, "rate": 15 }
}
```

Both fields are mandatory and must be integers. `rate: 0` means a bucket that never refills: `burst` requests, then refusal.

The profiles limit nothing on their own — they have to be assigned: through `ratelimit` in [`http`](#http), in [`websockets`](#websockets), or on an individual route method. Referring to a profile name that does not exist is a configuration error.

### http <Badge type="info" text="object"/>

HTTP routes, middleware and redirects. All four nested keys are optional.

#### ratelimit <Badge type="info" text="string"/>

The default rate-limiting profile for all of the vhost's HTTP traffic.

#### middlewares <Badge type="info" text="array of strings"/>

Middleware applied to every HTTP route. Names come from the application registry (`app/middlewares/middlewarelist.c`); an unknown name is a configuration error.

```json
"middlewares": ["middleware_http_auth"]
```

See [Middleware](/en/middleware).

#### routes <Badge type="info" text="object"/>

HTTP routes. The key is a path, the value an object of `METHOD → { handler }`.

Supported methods: **`GET`, `POST`, `PUT`, `DELETE`, `PATCH`, `HEAD`, `OPTIONS`**. Others (`CONNECT`, `TRACE`) are not declared in routes.

```json
"routes": {
    "/api/users": {
        "GET":  { "file": "handlers/models/lib_modeluser.so", "function": "list", "ratelimit": "strict" },
        "POST": { "file": "handlers/models/lib_modeluser.so", "function": "create" }
    },
    "/api/users/{id|\\d+}": {
        "PATCH": { "file": "handlers/models/lib_modeluser.so", "function": "update" }
    }
}
```

A path may be literal, carry named parameters (`{id|\d+}`) or be a regular expression — but **not both at once**. The syntax is covered in [Routing](/en/routing).

Handler fields:

* `file` — path to the `.so` holding the handler. The library is loaded at start-up and shared by every route that names it
* `function` — the exported function name. Resolved at start-up; not found means the server refuses to start
* `static_file` — a static file path. When present, `file`/`function` are **neither required nor called**: the route serves the file. The path is always resolved **relative to [`root`](#root)** — a leading `/` is stripped, it does not mean the filesystem root. Capture groups from the route's regular expression can be substituted as `{1}`, `{2}`, … (the same notation as `redirects`), so one route can serve a whole directory:
  ```json
  "/assets/(.*)": {
      "GET": {
          "static_file": "/assets/{1}",
          "cache_control": "public, max-age=31536000, immutable"
      }
  }
  ```
* `cache_control` — the `Cache-Control` header for whatever the route answers with, a file or a handler alike. Without it every file response carries `Cache-Control: no-cache` (revalidate on each use) — safe, but it makes the client re-download immutable build artefacts. Put immutable caching on routes whose files carry a content hash in the name, and leave pages on the default. A handler that sets its own `Cache-Control` keeps it — the route value is a default, not an override; and a missing `static_file` answers 404 without the header
* `ratelimit` — the rate-limiting profile for this method of this route, overriding `http.ratelimit`

A request matching no route is served as a static file from `root`.

#### redirects <Badge type="info" text="object"/>

Redirect rules. The key is a path or a regular expression, the value the target; capture groups are substituted as `{1}`, `{2}`, …:

```json
"redirects": {
    "/user": "/persons",
    "/user(.*)/(\\d)": "/user-{1}-{2}",
    "/section1/(\\d+)/section2/(\\d+)/section3": "/one/{1}/two/{2}/three"
}
```

Redirects are checked before routes.

### websockets <Badge type="info" text="object"/>

WebSocket configuration. All nested keys — `default`, `ratelimit`, `middlewares`, `routes` — are optional, but **the presence of the section itself is meaningful**.

```json
"websockets": {
    "default": { "file": "handlers/ws/lib_wsindex.so", "function": "default_" },
    "ratelimit": "strict",
    "middlewares": ["middleware_ws_auth"],
    "routes": {
        "/": { "GET": { "file": "handlers/ws/lib_wsindex.so", "function": "echo" } }
    }
}
```

* `default` — the handler for frames matching no route. Without it the built-in default handler is used
* `ratelimit`, `middlewares` — as in `http`, but for WebSocket traffic
* `routes` — routes; the method key here is also `GET`

::: tip The section also governs HTTP/2
A vhost **without** a `websockets` section answers Extended CONNECT (WebSocket over HTTP/2, RFC 8441) with `501 Not Implemented`. The capability is advertised per connection while serving is per vhost, so an empty `"websockets": {}` is a meaningful "WebSocket is allowed here".
:::

See [WebSocket requests](/en/wsrequests) and [Broadcasting](/en/wsbroadcast).

### tls <Badge type="info" text="object"/>

TLS/SSL settings. **All three fields are mandatory** when the section is present; an empty string is not allowed.

```json
"tls": {
    "fullchain": "/path/to/fullchain.pem",
    "private": "/path/to/privkey.pem",
    "ciphers": "TLS_AES_256_GCM_SHA384 TLS_CHACHA20_POLY1305_SHA256 ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-GCM-SHA384"
}
```

* `fullchain` — PEM certificate chain
* `private` — PEM private key
* `ciphers` — cipher list: TLS 1.3 suites separated by spaces first, then the TLS 1.2 list separated by colons

The vhost is selected by SNI on TLS, and the certificate comes from the matching vhost. HTTP/2 requires an AEAD cipher with ephemeral key exchange (RFC 9113 §9.2.2) — a connection on a weak cipher gets `GOAWAY(INADEQUATE_SECURITY)`, while HTTP/1.1 on the same cipher keeps working.

See [SSL certificates](/en/ssl-certs).

### http3 <Badge type="info" text="object"/>

Enables HTTP/3 (over QUIC) for the vhost. Requires the `-DINCLUDE_HTTP3=yes` build flag (OpenSSL ≥ 3.5) and a `tls` section — QUIC has no cleartext mode; `http3.enabled` without `tls` stops start-up, as does `enabled` in a build without HTTP/3 support. TCP keeps serving HTTP/1.1 and HTTP/2 alongside.

```json
"http3": {
    "enabled": true,
    "port": 443,
    "alt_svc": true,
    "alt_svc_max_age": 86400
}
```

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `enabled` | bool | `false` | Turns HTTP/3 on. Required for h3 to work |
| `port` | number | the vhost's TCP port | UDP port for QUIC (1–65535) |
| `alt_svc` | bool | `true` | Advertise h3 in the `Alt-Svc` header over HTTP/1.1 and HTTP/2 |
| `alt_svc_max_age` | number | `86400` | `Alt-Svc` cache lifetime (seconds, ≥ 0) |

Only the port goes into `Alt-Svc` (`h3=":443"; ma=86400`), never a host name: an empty host per RFC 7838 means "the same one", which keeps the header correct for every vhost sharing the listener.

QUIC's own behaviour is tuned by the `http3_*` keys in [`main.env`](#runtime-keys) — those are per process, not per vhost. See [HTTP/3](/en/http3).

## databases section

Database connections. Optional. Each driver is a **non-empty array** of hosts; in code a connection is addressed as `<driver>.<host_id>` (e.g. `postgresql.p1`, `redis.r1`, `sqlite.local`).

Only the drivers enabled at build time are compiled in (`-DINCLUDE_POSTGRESQL=yes` and so on). A driver named in the config but missing from the build is **skipped with a message in the journal** — start-up continues, but code addressing it will not find the host.

In every driver an **unknown field is a configuration error**, as is repeating a field twice.

### postgresql

```json
"postgresql": [{
    "host_id": "p1",
    "ip": "127.0.0.1",
    "port": 5432,
    "dbname": "mydb",
    "user": "dbuser",
    "password": "dbpass",
    "connection_timeout": 3,
    "schema": "public"
}]
```

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `host_id` | string | **yes** | Host identifier; addressed as `postgresql.<host_id>` |
| `ip` | string | **yes** | Database server address |
| `port` | number | **yes** | Port |
| `dbname` | string | **yes** | Database name |
| `user` | string | **yes** | User |
| `password` | string | **yes** | Password (may be an empty string) |
| `connection_timeout` | number | **yes** | Connection timeout, seconds |
| `schema` | string | no | Schema tables are looked up in. Unset means `current_schema()` |

### mysql

```json
"mysql": [{
    "host_id": "m1",
    "ip": "127.0.0.1",
    "port": 3306,
    "dbname": "mydb",
    "user": "dbuser",
    "password": "dbpass",
    "charset": "utf8mb4",
    "connection_timeout": 5
}]
```

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `host_id`, `ip`, `port`, `dbname`, `user`, `password` | — | **yes** | As for PostgreSQL |
| `charset` | string | no | Connection charset. Defaults to `utf8mb4` |
| `connection_timeout` | number | no | Connection timeout, seconds. `0` or absent means no timeout is set |

### redis

```json
"redis": [{
    "host_id": "r1",
    "ip": "127.0.0.1",
    "port": 6379,
    "dbindex": 0,
    "user": "",
    "password": ""
}]
```

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `host_id`, `ip`, `port` | — | **yes** | Identifier and address |
| `dbindex` | number | **yes** | Redis database index, **0–15** |
| `user` | string | no | User (Redis ACL) |
| `password` | string | no | Password |

### sqlite

```json
"sqlite": [{
    "host_id": "local",
    "path": "/var/lib/app/data.sqlite",
    "journal_mode": "WAL",
    "busy_timeout": 5000
}]
```

| Field | Type | Required | Default | Description |
|-------|------|----------|---------|-------------|
| `host_id` | string | **yes** | — | Host identifier |
| `path` | string | **yes** | — | Database file path; `":memory:"` for in-memory |
| `journal_mode` | string | no | `WAL` | `PRAGMA journal_mode` value |
| `busy_timeout` | number | no | `5000` | `PRAGMA busy_timeout` in ms; non-negative |

See [Databases](/en/db).

## storages section

Named file storages. Optional. The key name is what addresses the storage in code and in `sessions.storage_name`. The kind is set by `type`; an unknown type is a configuration error.

### filesystem

```json
"storages": {
    "local": {
        "type": "filesystem",
        "root": "/var/www/storage"
    }
}
```

`root` is mandatory and cannot be empty.

### s3

```json
"storages": {
    "remote": {
        "type": "s3",
        "access_id": "your_access_id",
        "access_secret": "your_access_secret",
        "protocol": "https",
        "host": "s3.amazonaws.com",
        "port": "443",
        "bucket": "my-bucket",
        "region": "us-east-1"
    }
}
```

::: warning Every field is mandatory and non-empty
`access_id`, `access_secret`, `protocol`, `host`, `port`, `bucket`, `region` — each must be a **non-empty string**. In particular `"port": ""` (seen in older examples) stops start-up: the port is given explicitly, as a string — `"443"` or `"80"`.
:::

See [Storages](/en/storage).

## sessions section

A named set of session configurations. Optional. Each key is the session name used in code (`session_create("backend", data, ttl)`).

Every session is encrypted with **AES-256-GCM**; the key is derived from the mandatory `secret` field. The driver is set by `driver`:

| `driver` | Required storage field | Description |
|----------|------------------------|-------------|
| `filesystem` | `storage_name` | Files in the named storage (a name from [`storages`](#storages-section)) |
| `redis` | `host_id` | Redis, addressed as `redis.<host_id>` |
| `database` | `host_id` | A database, addressed as `<driver>.<host_id>` (e.g. `postgresql.p1`) |

```json
"sessions": {
    "backend": {
        "driver": "filesystem",
        "storage_name": "local",
        "secret": "change-me"
    },
    "scheduler": {
        "driver": "redis",
        "host_id": "redis.r1",
        "secret": "change-me"
    },
    "doc-editor": {
        "driver": "database",
        "host_id": "postgresql.p1",
        "secret": "change-me"
    }
}
```

An unknown driver, a missing `secret` or an empty storage field is a configuration error. Session lifetime is not configured here — it is passed at creation: `session_create(name, data, duration)`.

See [Sessions](/en/session).

## mail section

Email delivery with DKIM signing. Optional; an absent section is equivalent to empty values — mail goes out unsigned.

```json
"mail": {
    "dkim_private": "/path/to/dkim_private.pem",
    "dkim_selector": "mail",
    "host": "example.com"
}
```

| Field | Description |
|-------|-------------|
| `dkim_private` | **Path** to the private key file. The file is read when the configuration loads — an unreadable path stops start-up |
| `dkim_selector` | DKIM selector (part of the `<selector>._domainkey.<host>` TXT record name) |
| `host` | The domain messages are signed for |

Each field is individually optional, but when present it must be a non-empty string. See [Mail](/en/mail).

## mimetypes section

**Required and non-empty.** A mapping of MIME types to file extensions: the key is a type, the value a non-empty array of extensions (without the dot).

```json
"mimetypes": {
    "text/html": ["html", "htm", "shtml"],
    "text/css": ["css"],
    "application/json": ["json"],
    "application/javascript": ["js"],
    "image/png": ["png"],
    "image/jpeg": ["jpeg", "jpg"],
    "application/octet-stream": ["bin", "exe", "dll"]
}
```

Two tables are built: "type → extension" and "extension → type". Only the **first** extension of an array goes into the former — that one becomes canonical for the type; all of them go into the latter. So an extension may appear under several types (the last occurrence wins), and the order inside an array matters.

A file whose extension is not described here is served without a meaningful `Content-Type`, so it pays to keep the table complete. The list in the repository example mirrors nginx's and is a reasonable starting point.

## Full configuration example

```json
{
    "main": {
        "workers": 4,
        "threads": 2,
        "reload": "hard",
        "env_file": "secrets/.env.production",
        "client_max_body_size": 110485760,
        "tmp": "/tmp",
        "gzip": ["text/html", "text/css", "application/json", "application/javascript"],
        "log": { "enabled": true, "level": "info" },
        "env": {
            "refresh_token_expiration": 15552000,
            "metrics": true,
            "gzip_static": true,
            "gzip_cache_size": 33554432,
            "gzip_cache_max_file": 1048576,
            "http2_idle_timeout_sec": 60,
            "http2_ping_interval_sec": 30,
            "http3_idle_timeout_sec": 300,
            "http3_keepalive_sec": 10
        }
    },
    "translations": [
        { "domain": "backend", "path": "/app/locale" }
    ],
    "task_manager": [
        {
            "name": "cleanup_expired_tokens",
            "type": "interval",
            "interval": 60,
            "file": "/app/build/exec/handlers/tasks/libtasks.so",
            "function": "cleanup_authorization_codes"
        },
        {
            "name": "nightly_report",
            "type": "daily",
            "hour": 3,
            "minute": 30,
            "file": "/app/build/exec/handlers/tasks/libtasks.so",
            "function": "send_report"
        }
    ],
    "servers": {
        "site_v4": {
            "domains": ["example.com", "*.example.com"],
            "ip": "0.0.0.0",
            "port": 443,
            "root": "/var/www/html",
            "index": "index.html",
            "ratelimits": {
                "default": { "burst": 15, "rate": 15 },
                "strict":  { "burst": 1,  "rate": 0  }
            },
            "http": {
                "ratelimit": "default",
                "middlewares": ["middleware_http_auth"],
                "routes": {
                    "/api/users": {
                        "GET":  { "file": "/app/build/exec/handlers/models/lib_modeluser.so", "function": "list" },
                        "POST": { "file": "/app/build/exec/handlers/models/lib_modeluser.so", "function": "create", "ratelimit": "strict" }
                    },
                    "/assets/(.*)": {
                        "GET": {
                            "static_file": "/assets/{1}",
                            "cache_control": "public, max-age=31536000, immutable"
                        }
                    },
                    "/robots.txt": {
                        "GET": { "static_file": "/robots.txt" }
                    }
                },
                "redirects": {
                    "/old": "/new",
                    "/user(.*)/(\\d)": "/user-{1}-{2}"
                }
            },
            "websockets": {
                "default": { "file": "/app/build/exec/handlers/ws/lib_wsindex.so", "function": "default_" },
                "ratelimit": "default",
                "middlewares": ["middleware_ws_auth"],
                "routes": {
                    "/ws": { "GET": { "file": "/app/build/exec/handlers/ws/lib_wsindex.so", "function": "connect" } }
                }
            },
            "tls": {
                "fullchain": "/etc/letsencrypt/live/example.com/fullchain.pem",
                "private": "/etc/letsencrypt/live/example.com/privkey.pem",
                "ciphers": "TLS_AES_256_GCM_SHA384 TLS_CHACHA20_POLY1305_SHA256"
            },
            "http3": {
                "enabled": true,
                "port": 443,
                "alt_svc": true,
                "alt_svc_max_age": 86400
            }
        },
        "site_v6": {
            "domains": ["example.com", "*.example.com"],
            "ip": "::",
            "port": 443,
            "root": "/var/www/html",
            "index": "index.html",
            "tls": {
                "fullchain": "/etc/letsencrypt/live/example.com/fullchain.pem",
                "private": "/etc/letsencrypt/live/example.com/privkey.pem",
                "ciphers": "TLS_AES_256_GCM_SHA384 TLS_CHACHA20_POLY1305_SHA256"
            },
            "http3": { "enabled": true }
        }
    },
    "databases": {
        "postgresql": [{
            "host_id": "p1", "ip": "127.0.0.1", "port": 5432,
            "dbname": "mydb", "user": "dbuser", "password": "dbpass",
            "connection_timeout": 3,
            "schema": "public"
        }],
        "mysql": [{
            "host_id": "m1", "ip": "127.0.0.1", "port": 3306,
            "dbname": "mydb", "user": "dbuser", "password": "dbpass",
            "charset": "utf8mb4",
            "connection_timeout": 5
        }],
        "redis": [{
            "host_id": "r1", "ip": "127.0.0.1", "port": 6379,
            "dbindex": 0, "user": "", "password": ""
        }],
        "sqlite": [{
            "host_id": "local", "path": "/var/lib/app/data.sqlite",
            "journal_mode": "WAL",
            "busy_timeout": 5000
        }]
    },
    "storages": {
        "local": {
            "type": "filesystem",
            "root": "/var/www/storage"
        },
        "remote": {
            "type": "s3",
            "access_id": "your_access_id",
            "access_secret": "your_access_secret",
            "protocol": "https",
            "host": "s3.amazonaws.com",
            "port": "443",
            "bucket": "my-bucket",
            "region": "us-east-1"
        }
    },
    "sessions": {
        "backend": {
            "driver": "filesystem",
            "storage_name": "local",
            "secret": "change-me"
        },
        "scheduler": {
            "driver": "redis",
            "host_id": "redis.r1",
            "secret": "change-me"
        },
        "doc-editor": {
            "driver": "database",
            "host_id": "postgresql.p1",
            "secret": "change-me"
        }
    },
    "mail": {
        "dkim_private": "/etc/dkim/private.pem",
        "dkim_selector": "mail",
        "host": "example.com"
    },
    "mimetypes": {
        "text/html": ["html", "htm"],
        "text/css": ["css"],
        "application/json": ["json"],
        "application/javascript": ["js"],
        "image/png": ["png"],
        "image/jpeg": ["jpeg", "jpg"],
        "image/svg+xml": ["svg"],
        "font/woff2": ["woff2"]
    }
}
```
