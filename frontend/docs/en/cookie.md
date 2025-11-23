---
outline: deep
description: Working with Cookies in C Web Framework. Reading, setting, secure, httpOnly and sameSite flags for secure session handling.
---

# Cookie

Cookies allow data to be saved between HTTP requests. The framework supports all modern security attributes: secure, httpOnly, and sameSite.

## Reading cookies

You can get cookies from the current request like this:

```C
void get(httpctx_t* ctx) {
    const char* token = ctx->request->get_cookie(ctx->request, "token");

    if (token == NULL) {
        ctx->response->send_data(ctx->response, "Token not found");
        return;
    }

    ctx->response->send_data(ctx->response, token);
}
```

## Sending cookies

```C
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
