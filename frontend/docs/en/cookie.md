---
outline: deep
description: Working with Cookies in C Web Framework. The cookie_t struct, reading and setting cookies, Secure, HttpOnly, SameSite flags and cookie deletion.
---

# Cookie

Cookies let you persist data across HTTP requests and are most commonly used for sessions, access tokens, and user preferences. The framework supports all modern security attributes: `Secure`, `HttpOnly`, and `SameSite`.

## Cookie structure

A cookie is described by the `cookie_t` struct (defined in `httpresponse.h`):

```c
typedef struct {
    const char* name;      // cookie name (required)
    const char* value;     // value (required)
    int seconds;           // lifetime in seconds
    const char* path;      // path (NULL to skip)
    const char* domain;    // domain (NULL to skip)
    int secure;            // 1 = HTTPS only
    int http_only;         // 1 = not accessible from JavaScript
    const char* same_site; // "Strict" | "Lax" | "None" (NULL to skip)
} cookie_t;
```

| Field | Type | Required | Behavior |
|-------|------|----------|----------|
| `name` | `const char*` | yes | If `NULL` or an empty string, the cookie is ignored. |
| `value` | `const char*` | yes | If `NULL`, the cookie is ignored. |
| `seconds` | `int` | no | `0` deletes the cookie (`Max-Age=0`); `> 0` sets the lifetime via `Expires`. |
| `path` | `const char*` | no | `NULL` or empty — the `Path` attribute is not added. |
| `domain` | `const char*` | no | `NULL` or empty — the `Domain` attribute is not added. |
| `secure` | `int` | no | `1` adds the `Secure` flag, `0` skips it. |
| `http_only` | `int` | no | `1` adds the `HttpOnly` flag, `0` skips it. |
| `same_site` | `const char*` | no | `NULL` or empty — the `SameSite` attribute is not added. |

::: tip How the Set-Cookie header is formed
When `seconds > 0`, the framework computes the expiry date (`current time + seconds`) and adds the `Expires` attribute in RFC 1123 format (`Wed, 21 Jun 2026 12:34:56 GMT`). The `Max-Age` attribute is **not** added. To delete a cookie immediately, set `seconds = 0` — this sends `Max-Age=0`.
:::

## Reading cookies

The `get_cookie` method returns a cookie's value by name from the current request. It is called through the request object.

```c
void get(httpctx_t* ctx) {
    const char* token = ctx->request->get_cookie(ctx->request, "token");

    if (token == NULL) {
        ctx->response->send_data(ctx->response, "Token not found");
        return;
    }

    ctx->response->send_data(ctx->response, token);
}
```

**Parameters**\
`name` — cookie name. Matching is case-insensitive.

**Return value**\
Pointer to the cookie value, or `NULL` if no cookie with that name is present in the request. The pointer is owned by the request — do not free it.

## Setting cookies

The `add_cookie` method adds a cookie to the response — it builds the `Set-Cookie` header and attaches it to the response. It is called through the response object.

```c
void get(httpctx_t* ctx) {
    ctx->response->add_cookie(ctx->response, (cookie_t){
        .name = "token",
        .value = "token_value",
        .seconds = 3600,
        .path = "/",
        .domain = ".example.com",
        .secure = 1,
        .http_only = 1,
        .same_site = "Lax"
    });

    ctx->response->send_data(ctx->response, "Token successfully added");
}
```

**Parameters**\
`cookie` — a `cookie_t` struct. The `name` and `value` fields are required; if either is missing, the cookie is ignored.

## Deleting a cookie

There is no separate deletion function — a cookie is removed by setting `seconds = 0`. This sends `Max-Age=0`, and the browser discards the cookie immediately. The name and `path` must match those the cookie was set with.

```c
void logout(httpctx_t* ctx) {
    ctx->response->add_cookie(ctx->response, (cookie_t){
        .name = "token",
        .value = "",
        .seconds = 0,
        .path = "/"
    });

    ctx->response->send_data(ctx->response, "Logged out");
}
```

## SameSite

The `SameSite` attribute controls whether a cookie is sent on cross-site requests:

| Value | Behavior |
|-------|----------|
| `Strict` | The cookie is sent only on same-site navigations. |
| `Lax` | Sent on top-level navigations to the site (the default in most browsers). |
| `None` | Sent on all requests, including cross-site. Requires the `Secure` flag. |

## Security

- **`Secure`** — the cookie is transmitted only over HTTPS. Always enable it for authentication cookies.
- **`HttpOnly`** — hides the cookie from JavaScript (`document.cookie`), protecting it from XSS theft. Enable it for session cookies.
- **`SameSite`** — protects against CSRF. `Lax` suits most applications; for cookies needed in iframes or cross-site requests, use `None` together with `Secure`.

A complete example of using cookies together with sessions is in the [Authentication](./auth.md) section.
