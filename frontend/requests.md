---
outline: deep
description: Обработка HTTP-запросов в C Web Framework. Извлечение параметров, cookie, заголовков, работа с payload и JSON.
---

# HTTP-запросы

HTTP-запросы обрабатываются через контекст `httpctx_t`, который предоставляет доступ к объектам `request` и `response`.

## Базовая структура обработчика

```c
#include "http.h"

void my_handler(httpctx_t* ctx) {
    // ctx->request — объект запроса
    // ctx->response — объект ответа

    ctx->response->send_data(ctx->response, "Hello World");
}
```

## Параметры GET-запроса

Параметры строки запроса извлекаются методом `query()`:

```c
void get_users(httpctx_t* ctx) {
    const char* id = ctx->request->query(ctx->request, "id");
    const char* name = ctx->request->query(ctx->request, "name");

    if (id) {
        ctx->response->send_data(ctx->response, id);
        return;
    }

    ctx->response->send_data(ctx->response, "id not found");
}
```

## Параметры маршрута

Динамические параметры из URL также доступны через `query()`:

```json
// config.json
"/api/users/{id|\\d+}": {
    "GET": { "file": "handlers/libapi.so", "function": "get_user" }
}
```

```c
void get_user(httpctx_t* ctx) {
    const char* id = ctx->request->query(ctx->request, "id");
    // id содержит значение из URL, например "123" для /api/users/123
}
```

## Данные POST/PUT/PATCH запросов

Данные тела запроса доступны через методы payload:

### Получение всего тела запроса

```c
void post_data(httpctx_t* ctx) {
    char* payload = ctx->request->get_payload(ctx->request);

    if (!payload) {
        ctx->response->send_data(ctx->response, "No payload");
        return;
    }

    ctx->response->send_data(ctx->response, payload);
    free(payload);
}
```

### Получение поля по ключу

Для `multipart/form-data` и `application/x-www-form-urlencoded`:

```c
void post_form(httpctx_t* ctx) {
    char* email = ctx->request->get_payloadf(ctx->request, "email");
    char* password = ctx->request->get_payloadf(ctx->request, "password");

    if (!email || !password) {
        ctx->response->send_data(ctx->response, "Missing fields");
        if (email) free(email);
        if (password) free(password);
        return;
    }

    // Обработка данных...

    free(email);
    free(password);
}
```

### Получение JSON из тела запроса

```c
#include "json.h"

void post_json(httpctx_t* ctx) {
    json_doc_t* doc = ctx->request->get_payload_json(ctx->request);

    if (!json_ok(doc)) {
        ctx->response->send_data(ctx->response, "Invalid JSON");
        json_free(doc);
        return;
    }

    json_token_t* root = json_root(doc);
    json_token_t* name = json_object_get(root, "name");

    if (name && json_is_string(name)) {
        ctx->response->send_data(ctx->response, json_string(name));
    }

    json_free(doc);
}
```

Подробнее о работе с payload: [Получение данных от клиента](/payload)

## HTTP-метод запроса

Текущий метод доступен через `ctx->request->method`:

```c
void handler(httpctx_t* ctx) {
    switch (ctx->request->method) {
        case ROUTE_GET:
            // GET запрос
            break;
        case ROUTE_POST:
            // POST запрос
            break;
        case ROUTE_PUT:
            // PUT запрос
            break;
        case ROUTE_PATCH:
            // PATCH запрос
            break;
        case ROUTE_DELETE:
            // DELETE запрос
            break;
        case ROUTE_OPTIONS:
            // OPTIONS запрос
            break;
    }
}
```

## URL запроса

Свойства для работы с URL:

| Свойство | Описание | Пример |
|----------|----------|--------|
| `uri` | Полный URI с параметрами | `/users?id=100` |
| `path` | Путь без параметров | `/users` |
| `ext` | Расширение файла | `html` |
| `uri_length` | Длина URI | — |
| `path_length` | Длина пути | — |
| `ext_length` | Длина расширения | — |

```c
void handler(httpctx_t* ctx) {
    // Проверка длины URI
    if (ctx->request->uri_length > 4096) {
        ctx->response->send_default(ctx->response, 414); // URI Too Long
        return;
    }

    ctx->response->send_data(ctx->response, ctx->request->path);
}
```

## HTTP-заголовки

Получение заголовков запроса:

```c
void handler(httpctx_t* ctx) {
    http_header_t* host = ctx->request->get_header(ctx->request, "Host");
    http_header_t* auth = ctx->request->get_header(ctx->request, "Authorization");

    if (host) {
        printf("Host: %s\n", host->value);
    }

    if (auth) {
        // Проверка авторизации...
    }
}
```

## Cookie

Чтение cookie из запроса:

```c
void handler(httpctx_t* ctx) {
    const char* token = ctx->request->get_cookie(ctx->request, "session_token");

    if (!token) {
        ctx->response->send_data(ctx->response, "Not authorized");
        return;
    }

    // Валидация токена...
}
```

Подробнее: [Cookie](/cookie)

## Соединение с базой данных

Доступ к базам данных через контекст:

```c
#include "db.h"

void handler(httpctx_t* ctx) {
    dbinstance_t dbinst = dbinstance(ctx->request->database_list(ctx->request), "postgresql");

    if (!dbinst.ok) {
        ctx->response->send_data(ctx->response, "Database not available");
        return;
    }

    // Выполнение запросов...
}
```

Подробнее: [База данных](/db)
