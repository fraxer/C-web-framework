---
outline: deep
description: Формирование HTTP-ответов в C Web Framework. Коды состояния, заголовки, JSON, редиректы, отправка файлов и моделей.
---

# HTTP-ответы

HTTP-ответы формируются через объект `response` в контексте `httpctx_t`. Ответ содержит код состояния, заголовки и тело.

## Код состояния

По умолчанию код состояния равен `200`. Для изменения:

```c
void handler(httpctx_t* ctx) {
    ctx->response->statusCode = 201; // Created
    ctx->response->send_data(ctx->response, "Resource created");
}
```

### Стандартные ответы

Метод `send_default()` отправляет стандартный ответ с указанным кодом:

```c
void handler(httpctx_t* ctx) {
    if (error) {
        ctx->response->send_default(ctx->response, 500); // Internal Server Error
        return;
    }

    if (!found) {
        ctx->response->send_default(ctx->response, 404); // Not Found
        return;
    }

    ctx->response->send_data(ctx->response, "Success");
}
```

Полный список: [HTTP-коды](/http-codes)

## HTTP-заголовки

Добавление заголовков ответа:

```c
void handler(httpctx_t* ctx) {
    ctx->response->add_header(ctx->response, "Content-Type", "application/json");
    ctx->response->add_header(ctx->response, "X-Custom-Header", "value");

    ctx->response->send_data(ctx->response, "{\"status\": \"ok\"}");
}
```

## Отправка данных

### Текстовые данные

```c
void handler(httpctx_t* ctx) {
    ctx->response->send_data(ctx->response, "Hello World!");
}
```

### JSON-ответ

```c
#include "json.h"

void handler(httpctx_t* ctx) {
    json_doc_t* doc = json_init();
    json_token_t* root = json_create_object(doc);

    json_object_set(root, "status", json_create_string(doc, "success"));
    json_object_set(root, "code", json_create_int(doc, 200));

    ctx->response->add_header(ctx->response, "Content-Type", "application/json");
    ctx->response->send_data(ctx->response, json_stringify(doc));

    json_free(doc);
}
```

### Отправка модели

Модели можно отправлять напрямую с выбором полей:

```c
#include "user.h"

void get_user(httpctx_t* ctx) {
    user_t* user = user_find_by_id(123);

    if (!user) {
        ctx->response->send_default(ctx->response, 404);
        return;
    }

    // Отправка модели как JSON с выбранными полями
    ctx->response->send_model(ctx->response, user,
                              display_fields("id", "email", "name"));

    user_free(user);
}
```

## Редиректы

Перенаправление на другой URL:

```c
void handler(httpctx_t* ctx) {
    // 301 — постоянный редирект
    ctx->response->redirect(ctx->response, "/new-location", 301);
}

void temp_redirect(httpctx_t* ctx) {
    // 302 — временный редирект
    ctx->response->redirect(ctx->response, "/temporary", 302);
}
```

## Отправка файлов

### Статический файл

```c
void download(httpctx_t* ctx) {
    ctx->response->send_file(ctx->response, "/path/to/file.pdf");
}
```

### Файл с хранилища

```c
#include "storage.h"

void download_from_storage(httpctx_t* ctx) {
    file_t file = storage_file_get("local", "/uploads/document.pdf");

    if (!file.ok) {
        ctx->response->send_default(ctx->response, 404);
        return;
    }

    ctx->response->send_filef(ctx->response, "local", "/uploads/%s", "document.pdf");
    file.close(&file);
}
```

## Cookie

Установка cookie в ответе:

```c
void login(httpctx_t* ctx) {
    // ... проверка учетных данных ...

    ctx->response->add_cookie(ctx->response, (cookie_t){
        .name = "session_token",
        .value = "abc123xyz",
        .seconds = 3600,         // Время жизни в секундах
        .path = "/",
        .domain = ".example.com",
        .secure = 1,             // Только HTTPS
        .http_only = 1,          // Недоступна из JavaScript
        .same_site = "Lax"       // Защита от CSRF
    });

    ctx->response->send_data(ctx->response, "Logged in");
}
```

Удаление cookie:

```c
void logout(httpctx_t* ctx) {
    ctx->response->add_cookie(ctx->response, (cookie_t){
        .name = "session_token",
        .value = "",
        .seconds = -1,  // Истекшее время
        .path = "/"
    });

    ctx->response->redirect(ctx->response, "/login", 302);
}
```

## CORS-заголовки

```c
void api_handler(httpctx_t* ctx) {
    ctx->response->add_header(ctx->response, "Access-Control-Allow-Origin", "*");
    ctx->response->add_header(ctx->response, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE");
    ctx->response->add_header(ctx->response, "Access-Control-Allow-Headers", "Content-Type, Authorization");

    if (ctx->request->method == ROUTE_OPTIONS) {
        ctx->response->statusCode = 204;
        ctx->response->send_data(ctx->response, "");
        return;
    }

    // Основная логика...
}
```

## Важно

- Ответ отправляется клиенту только после вызова `send_data()`, `send_file()`, `send_model()` или `redirect()`
- Вызывайте один из этих методов в конце каждого обработчика
- После отправки ответа обработчик должен завершиться (используйте `return`)
- Не вызывайте `send_data()` после `send_file()` — это приведет к некорректному ответу
