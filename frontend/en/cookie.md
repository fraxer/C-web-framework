---
outline: deep
description: Setting and removing cookies
---

# Cookie

Cookies allow user data to be saved between requests. Cpdy encapsulates cookies in the `http_cookie_t` structure, which makes it possible to access them through the `get_cookie` method and provides additional convenience in work.

## Get cookie

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

## Set cookie

```C
void get(httpctx_t* ctx) {
    ctx->response->add_cookie(ctx->response, (cookie_t){
        .name = "token",
        .value = "token_value",
        .seconds = 3600,
        .path = "/",
        .domain = ".example.com"
        .secure = 1,
        .http_only = 1,
        .same_site = "Lax"
    });

    ctx->response->send_data(ctx->response, "Token successfully added");
}
```
