---
outline: deep
description: Работа с Cookie в C Web Framework. Чтение, установка, secure, httpOnly и sameSite флаги для безопасной работы с сессиями.
---

# Cookie

Cookie позволяют сохранять данные между HTTP-запросами. Фреймворк поддерживает все современные атрибуты безопасности: secure, httpOnly и sameSite.

## Чтение cookie

Получить куки из текущего запроса можно следующим образом:

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

## Отправка cookie

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
