---
outline: deep
description: C Web Framework routing system. HTTP and WebSocket route configuration, dynamic parameters, middleware, rate limiting and redirects.
---

# Routing

Routing defines the mapping between URLs and handlers. Both HTTP and WebSocket protocols are supported. Routes are described in the `servers.<id>` section of `config.json` — inside the `http` and `websockets` objects.

## Basic configuration

The key of the `routes` object is a path; the value is a "method → handler" object:

```json
"http": {
    "routes": {
        "/": {
            "GET": { "file": "handlers/libindex.so", "function": "index" }
        },
        "/api/users": {
            "GET":  { "file": "handlers/libapi.so", "function": "get_users" },
            "POST": { "file": "handlers/libapi.so", "function": "create_user" }
        }
    }
}
```

Handler fields:

| Field         | Type   | Description                                                                          |
|---------------|--------|--------------------------------------------------------------------------------------|
| `file`        | string | Path to the handler `.so` (relative to the process root, or absolute)               |
| `function`    | string | Name of the handler function                                                         |
| `static_file` | string | Path to a static file; when set, the file is served instead of calling `file`/`function`. Supports `{1}`, `{2}`, … capture-group substitution from the route pattern |
| `storage`     | string | Name of a storage from the [`storages`](/en/storage) section that `static_file` is taken from, instead of the server's `root`. Only together with `static_file` — see [Serving from a storage](#serving-from-a-storage) |
| `cache_control` | string | `Cache-Control` header for what the route answers with, file or handler (file default: `no-cache`) |
| `ratelimit`   | string | Name of the rate limiting profile for this route (overrides `http.ratelimit`) |

## Route matching

Routes are checked **in declaration order** — the first match wins. There are two kinds of routes:

* **Primitive** — a path with no parameters and no regex operators (`/`, `/api/users`, `/api/v1.0`). Matched by a character-by-character comparison; when such a route does not match, the next one is tried straight away — no regular expression is run for it at all.
* **Parameterized / regex** — a path with `{...}` parameters or regex operators (`*`, `+`, `(`, `)`, `[`, `]`, `|`, `^`, `$`, `\`). Compiled into a PCRE regular expression.

If the matched route has no handler for the current method, matching continues with the remaining routes. If no route matches, a `404` is returned.

### Dots and question marks

In a **plain** path — one with no `{...}` parameters and not a single operator from the list above — a dot and a question mark stand for themselves: they are escaped when the location is parsed. The route `/api/v1.0` answers `/api/v1.0` only, and no longer matches `/api/v1x0`.

As soon as one operator appears in the path, the whole string is read as a pattern and the dot means "any character" again — otherwise `/assets/(.*)` would stop capturing filenames with an extension. The same holds for paths with parameters: in `/files/{name|[a-z]+}.json` the dot matches any character, so `/files/abcxjson` fits that route too. When you mean the dot character itself, escape it yourself (the backslash is doubled in JSON):

```json
"routes": {
    "/api/v1.0": {
        "GET": { "file": "handlers/libapi.so", "function": "v1_0" }
    },
    "/files/{name|[a-z]+}\\.json": {
        "GET": { "file": "handlers/libfiles.so", "function": "get_json" }
    }
}
```

## Dynamic parameters

Route parameters are specified in the format `{name|pattern}`, where `pattern` is a PCRE regular expression:

```json
"routes": {
    "/api/users/{id|\\d+}": {
        "GET":    { "file": "handlers/libapi.so", "function": "get_user" },
        "PUT":    { "file": "handlers/libapi.so", "function": "update_user" },
        "DELETE": { "file": "handlers/libapi.so", "function": "delete_user" }
    },
    "/api/posts/{slug|[a-z0-9-]+}": {
        "GET": { "file": "handlers/libapi.so", "function": "get_post" }
    }
}
```

Parameter values are added to the query string and read as query parameters.

### Extracting parameters in a handler

```c
#include "query.h"

void get_user(httpctx_t* ctx) {
    int ok = 0;
    const char* id = query_param_char(ctx->request->query_, "id", &ok);
    // id = "123" for request /api/users/123

    if (!ok) {
        // handle error
        return;
    }

    // Convert to number
    int user_id = atoi(id);

    // Find user...
}

void get_post(httpctx_t* ctx) {
    int ok = 0;
    const char* slug = query_param_char(ctx->request->query_, "slug", &ok);
    // slug = "my-first-post" for /api/posts/my-first-post
}
```

::: warning Named parameters and regular expressions
Named parameters `{name|pattern}` cannot be mixed with "raw" regular-expression characters (`* [ ] ( ) + ^ | $`) in the same path — the parameter itself is the regular expression. For free-form patterns, use full regular expressions (see below).
:::

## Regular expressions

For complex patterns, use full regular expressions. Capture groups are available in the handler as numbered parameters:

```json
"routes": {
    "^/managers/{name|[a-z]+}$": {
        "GET": { "file": "handlers/libapi.so", "function": "get_manager" }
    },
    "^/files/(.+)\\.pdf$": {
        "GET": { "file": "handlers/libfiles.so", "function": "get_pdf" }
    }
}
```

A route pattern may have at most 64 capture groups, counting the groups of `{name|…}` parameters. A route with more does not load, and the log says why.

## HTTP methods

Supported methods:

| Method     | Description                  |
|------------|------------------------------|
| `GET`      | Retrieve a resource          |
| `POST`     | Create a resource            |
| `PUT`      | Full update                  |
| `PATCH`    | Partial update               |
| `DELETE`   | Delete a resource            |
| `HEAD`     | Headers without body         |
| `OPTIONS`  | CORS preflight requests      |

```json
"/api/resource": {
    "GET":     { "file": "handlers/lib.so", "function": "get" },
    "POST":    { "file": "handlers/lib.so", "function": "create" },
    "PUT":     { "file": "handlers/lib.so", "function": "replace" },
    "PATCH":   { "file": "handlers/lib.so", "function": "update" },
    "DELETE":  { "file": "handlers/lib.so", "function": "delete" },
    "HEAD":    { "file": "handlers/lib.so", "function": "head" },
    "OPTIONS": { "file": "handlers/lib.so", "function": "options" }
}
```

## Static files

For routes that should serve static files without processing, use the `static_file` parameter instead of `file` and `function`:

```json
"routes": {
    "/": {
        "GET": { "static_file": "public/index.html" }
    },
    "/favicon.ico": {
        "GET": { "static_file": "public/favicon.ico" }
    },
    "/robots.txt": {
        "GET": { "static_file": "public/robots.txt" }
    }
}
```

The file path is relative to the server's `root` directory.

A `static_file` path may reference the route's capture groups with `{1}`, `{2}`, … (the same notation `redirects` use), which turns one route into a whole directory — handy for build artifacts whose names carry a content hash:

```json
"/assets/(.*)": {
    "GET": {
        "static_file": "/assets/{1}",
        "cache_control": "public, max-age=31536000, immutable"
    }
}
```

By default every file answer is sent with `Cache-Control: no-cache`, so the client revalidates on each use. `cache_control` overrides that per route — pair it with fingerprinted filenames and the browser stops asking for the file at all. Files reached through a route without `cache_control` keep the default.

`cache_control` applies to handler routes as well, where it acts as the route's default: a handler that sets `Cache-Control` itself keeps its own value. A `static_file` that is missing still answers 404 without the header, so a long `max-age` never lands on an error.

### Serving from a storage

By default `static_file` is resolved against the server's `root`. The `storage` key moves the route into a named storage from the [`storages`](/en/storage) section — filesystem and S3 alike. The route-to-storage binding becomes a line of configuration instead of a handler that programs it:

```json
"/assets/(.*)": {
    "GET": {
        "static_file": "{1}",
        "storage": "assets",
        "cache_control": "public, max-age=31536000, immutable"
    }
},
"/videos/(.*)": {
    "GET": { "static_file": "{1}", "storage": "media" }
}
```

The usual case: the markup sits next to the code on a fast disk, while keeping video there is expensive — `assets` points at a directory, `media` at an S3 bucket, and the client cannot tell.

A route without `storage` behaves exactly as before, relative to `root`.

The key is set per method, not per route: `GET` and `HEAD` of the same path each name it. A method without an entry skips this route, and if nothing further matches, the file is looked up in `root`.

#### How a storage route differs from ordinary statics

The same for a filesystem storage and for S3:

* **Route middlewares and the route's ratelimit run on it.** Middlewares do not run on statics served from `root`: a private video behind authorization becomes configuration rather than a handler, and a request into S3 costs egress that has to be limitable. The ratelimit is checked first (a refusal is `429` with `Retry-After`), then the middlewares; a request a middleware turns down gets that middleware's answer, and the storage is never asked.
* **The route's `cache_control` lands only on what the storage served** — `200`, `206` and `304`. Neither a `404`, nor a `429`, nor a middleware's refusal, nor an S3 error carries it, so a year-long `max-age` never caches an error.
* **The answer is prepared in a worker thread** from the `main.threads` pool, as a handler's is, rather than on the event loop: that is where the middlewares can run.

```json
"/private/(.*)": {
    "GET": {
        "static_file": "{1}",
        "storage": "media",
        "ratelimit": "downloads",
        "cache_control": "private, max-age=3600"
    }
}
```

Middlewares come from the vhost's shared `http.middlewares` list — a route has no list of its own. If other routes of the vhost do not need the same check, the middleware decides for itself, by the request path.

#### A filesystem storage

Serves the file the way statics from `root` do: `sendfile`, range requests, `ETag`, `gzip_static`. The path comes from the request, so patterns in it (`*`, `?`, `[`, `~`) are refused rather than expanded, and so is anything leaving the storage (`..`). A directory, a FIFO and any other non-regular file answer `404`.

#### An S3 storage

The object is asked for with `HEAD` first — that answers the size, `ETag`, `Last-Modified` and `Content-Type`, and it is also what serves the HTTP `HEAD` method, with no body downloaded. The body then arrives in 8 MB chunks, so the HTTP client's timeout is spent on a chunk rather than on the whole file. The chunk is further clamped by [`client_max_body_size`](/en/config#client-max-body-size): the client refuses a response above it.

* A client `Range` is cut by S3 itself — only what was asked for is pulled off an object of hundreds of megabytes. An open end (`bytes=100-`) is clipped to the chunk size: RFC 9110 §14.2 allows serving a subset of what was requested, and a player continues with explicit ranges.
* Several ranges in one header pull the whole object, and the ordinary machinery slices it afterwards — the answer arrives as `multipart/byteranges`.
* `If-None-Match` and `If-Modified-Since` are proxied to S3, and its `304` is relayed without fetching a body. `If-Range` is checked against the `ETag` locally.
* `gzip` is not applied to the S3 branch: `gzip_static` and the compressed cache are local machinery, and video is not in `main.gzip` anyway.
* A `403` from S3 reaches the client as `502`: it is the server's keys that are wrong, not the client's rights to the object. A timeout or an unreachable endpoint is `503`.

An S3 chunk occupies a worker thread from the shared `main.threads` pool — the same one handlers run in. With a narrow pool a slow S3 will slow the whole virtual host down.

::: warning The storage key requires static_file
`storage` without `static_file` is a configuration error, and so is `storage` together with `file`/`function`. An unknown storage name will not let the server start either: the name is checked against the `storages` section when the configuration is loaded.
:::

### Combining with handlers

Static files and handlers can be combined in the same route for different methods:

```json
"/api/docs": {
    "GET":  { "static_file": "public/api-docs.html" },
    "POST": { "file": "handlers/libapi.so", "function": "update_docs" }
}
```

### Rate limiting for static files

A `static_file` served from `root` uses the route's `ratelimit`, or `http.ratelimit` without one. The check runs before looking up the file: requests for missing files also spend tokens. An exhausted limit returns `429` with `Retry-After: 1`; the route's `cache_control` is not applied to that response.

A request that matches no route is also checked against `http.ratelimit` before looking up the file in `root`. Those requests and routes without their own profile spend tokens from the same client bucket: a series of requests for missing files can exhaust the limit for existing files too. Once the bucket refills, a missing file receives the usual `404` again.

If neither the route nor `http` has a profile assigned, there is no limit. A route profile with `rate: 0` disables limiting for that route even when `http.ratelimit` is set. Middlewares still do not run on statics served from `root` — rate limiting works independently of them.

A profile can be assigned directly to a file route:

```json
"/downloads/(.*)": {
    "GET": {
        "static_file": "{1}",
        "ratelimit": "downloads"
    }
}
```

::: tip When to use static_file
Use `static_file` for files that don't require processing: HTML pages, images, documents, favicon, etc. This is more efficient than creating a handler for each file.
:::

## Middleware

Middleware are executed before the handler and apply to all routes of the section (`http.middlewares` / `websockets.middlewares`). In HTTP these are handler routes and [storage routes](#serving-from-a-storage); statics from `root` — both through `static_file` and for a request that matched no route — are served without middlewares:

### Global middleware

```json
"http": {
    "middlewares": ["middleware_auth", "middleware_log"],
    "routes": {
        // All routes pass through middleware_auth and middleware_log
    }
}
```

### Middleware in a handler

```c
#include "httpmiddlewares.h"

void protected_handler(httpctx_t* ctx) {
    // Authentication check
    middleware(
        middleware_http_auth(ctx)
    )

    // This code will only execute if the user is authorized
    user_t* user = httpctx_get_user(ctx);
    ctx->response->send_data(ctx->response, "Welcome!");
}
```

## Rate Limiting

Rate limiting profiles are defined at the server level — in the `ratelimits` section (alongside `http` and `websockets`). Each profile defines `burst` (bucket capacity / peak number of requests) and `rate` (token refill rate per second):

```json
"s1": {
    "ratelimits": {
        "default": { "burst": 15,  "rate": 15 },
        "strict":  { "burst": 1,   "rate": 1 },
        "api":     { "burst": 100, "rate": 100 }
    },
    "http": {
        "ratelimit": "default",
        "routes": {
            "/api/users": {
                "GET":  { "file": "handlers/libapi.so", "function": "get_users",   "ratelimit": "api" },
                "POST": { "file": "handlers/libapi.so", "function": "create_user", "ratelimit": "api" }
            },
            "/api/login": {
                "POST": { "file": "handlers/libapi.so", "function": "login", "ratelimit": "strict" }
            }
        }
    }
}
```

* `http.ratelimit` — the default profile for all HTTP routes and statics from `root` served to a request that matched no route. Routes without their own profile and those statics share one bucket per client address.
* A `ratelimit` on a specific route overrides the profile for that route. The profile is declared inside a method but applies to the whole route; use separate routes for different limits.
* The limit applies to both a `static_file` served from `root` and [storage routes](#serving-from-a-storage). Requests for missing files also spend tokens. An exhausted limit returns HTTP `429` with `Retry-After: 1`.
* Redirects from `http.redirects` are handled before routing and do not spend tokens from this limiter.
* Each client address gets a bucket of its own. `rate: 0` turns the profile off — the limiter lets every request through; the strictest profile is `{ "burst": 1, "rate": 1 }`, see [ratelimits](/en/config#ratelimits).

## Redirects

Redirects are described in the `http.redirects` section. The key is a PCRE regular expression; the value is the target path. Capture groups are substituted via `{1}`, `{2}`, etc.:

```json
"http": {
    "redirects": {
        "/user": "/persons",
        "/user(.*)/(\\d)": "/user-{1}-{2}",
        "/section1/(\\d+)/section2/(\\d+)/section3": "/one/{1}/two/{2}/three"
    }
}
```

A redirect location is **not anchored**, and it is matched against the request path without the query string: `"/user": "/persons"` fires on any path that contains `/user` — including `/api/user/42`. To have a redirect answer one exact path, anchor it: `"^/user$"`.

A redirect location, like a route pattern, may have at most 64 capture groups.

## WebSocket routes

WebSocket is configured in the `websockets` section. It supports a default handler (`default`), a shared `ratelimit`, `middlewares` and `routes`:

```json
"websockets": {
    "default": {
        "file": "handlers/libws.so",
        "function": "ws_default"
    },
    "ratelimit": "default",
    "routes": {
        "/ws": {
            "GET":  { "file": "handlers/libws.so", "function": "ws_connect" },
            "POST": { "file": "handlers/libws.so", "function": "ws_message" }
        },
        "/ws/chat/{room|\\d+}": {
            "GET":  { "file": "handlers/libws.so", "function": "ws_join_room" },
            "POST": { "file": "handlers/libws.so", "function": "ws_send_message" }
        }
    }
}
```

* `default` — the handler called when no route matches. If the `websockets` section is not set, a built-in default handler is used.
* `ratelimit` — the default rate limiting profile for all WS routes.
* `middlewares` — middleware applied to all WS routes.

Supported methods for WebSocket routes: `GET`, `POST`, `PATCH`, `DELETE`.

### WebSocket handler

```c
#include "websockets.h"

void ws_connect(wsctx_t* ctx) {
    ctx->response->send_text(ctx->response, "Connected");
}

void ws_message(wsctx_t* ctx) {
    char* message = websocketsrequest_payload(ctx->request->protocol);

    if (message) {
        ctx->response->send_text(ctx->response, message);
        free(message);
    }
}
```

## Configuration examples

### REST API

```json
"http": {
    "middlewares": ["middleware_cors"],
    "ratelimit": "default",
    "routes": {
        "/api/v1/users": {
            "GET":  { "file": "handlers/libusers.so", "function": "list" },
            "POST": { "file": "handlers/libusers.so", "function": "create" }
        },
        "/api/v1/users/{id|\\d+}": {
            "GET":    { "file": "handlers/libusers.so", "function": "show" },
            "PUT":    { "file": "handlers/libusers.so", "function": "update" },
            "DELETE": { "file": "handlers/libusers.so", "function": "delete" }
        },
        "/api/v1/auth/login": {
            "POST": { "file": "handlers/libauth.so", "function": "login" }
        },
        "/api/v1/auth/logout": {
            "POST": { "file": "handlers/libauth.so", "function": "logout" }
        }
    }
}
```

### Statics in storages

Markup in a directory on disk, video in S3, downloads behind authorization and a rate limit:

```json
"storages": {
    "assets": { "type": "filesystem", "root": "/var/www/assets" },
    "media": {
        "type": "s3", "access_id": "...", "access_secret": "...",
        "protocol": "https", "host": "s3.example.com", "port": "",
        "bucket": "media", "region": "us-east-1"
    }
},
"servers": {
    "s1": {
        "ratelimits": { "downloads": { "burst": 5, "rate": 1 } },
        "http": {
            "middlewares": ["middleware_auth"],
            "routes": {
                "/assets/(.*)": {
                    "GET": {
                        "static_file": "{1}",
                        "storage": "assets",
                        "cache_control": "public, max-age=31536000, immutable"
                    }
                },
                "/videos/(.*)": {
                    "GET":  { "static_file": "{1}", "storage": "media", "ratelimit": "downloads" },
                    "HEAD": { "static_file": "{1}", "storage": "media" }
                }
            }
        }
    }
}
```

`middleware_auth` runs here for both `/assets/` and `/videos/`: the middleware list is shared by the vhost. A middleware that should guard only the videos checks the request path itself.

### Mixed site (pages + API)

```json
"http": {
    "routes": {
        "/": {
            "GET": { "file": "handlers/libpages.so", "function": "home" }
        },
        "/about": {
            "GET": { "file": "handlers/libpages.so", "function": "about" }
        },
        "/api/data": {
            "GET": { "file": "handlers/libapi.so", "function": "get_data" }
        }
    }
}
```
