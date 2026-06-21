---
outline: deep
description: Обработка HTTP-запросов в C Web Framework. Структура запроса, метод, URL, query-параметры, заголовки, cookie, тело и JSON.
---

# HTTP-запросы

HTTP-запросы обрабатываются через контекст `httpctx_t`. Он содержит указатели на объекты запроса и ответа, а также слот для произвольных пользовательских данных.

```c
typedef struct httpctx {
    httprequest_t* request;   // объект запроса
    httpresponse_t* response; // объект ответа
    void* user_data;          // произвольные данные (например, аутентифицированный пользователь)
} httpctx_t;
```

## Базовая структура обработчика

```c
#include "http.h"

void my_handler(httpctx_t* ctx) {
    ctx->response->send_data(ctx->response, "Hello World");
}
```

Методы запроса и ответа — это указатели на функции, навешанные на объект. Первый аргумент всегда сам объект: `ctx->request->get_header(ctx->request, "Host")`.

## Поля запроса

Основные поля объекта `httprequest_t`:

| Поле | Тип | Описание |
|------|-----|----------|
| `uri` | `const char*` | Полный URI с query-строкой: `/users?id=100` |
| `path` | `const char*` | Путь без query-строки: `/users` |
| `uri_length` | `size_t` | Длина `uri` |
| `path_length` | `size_t` | Длина `path` |
| `method` | `route_methods_e` | HTTP-метод запроса |
| `version` | `http_version_e` | Версия протокола (HTTP/1.0, HTTP/1.1) |
| `query_` | `query_t*` | Связный список query-параметров и параметров маршрута |

```c
void handler(httpctx_t* ctx) {
    // Ограничение длины URI
    if (ctx->request->uri_length > 4096) {
        ctx->response->send_default(ctx->response, 414); // URI Too Long
        return;
    }

    ctx->response->send_data(ctx->response, ctx->request->path);
}
```

## HTTP-метод

Текущий метод доступен через `ctx->request->method` и представлен значениями enum `route_methods_e` (`core/src/route/route.h`):

```c
void handler(httpctx_t* ctx) {
    switch (ctx->request->method) {
        case ROUTE_GET:     // GET
        case ROUTE_HEAD:    // HEAD
        case ROUTE_POST:    // POST
        case ROUTE_PUT:     // PUT
        case ROUTE_PATCH:   // PATCH
        case ROUTE_DELETE:  // DELETE
        case ROUTE_OPTIONS: // OPTIONS
            break;
        case ROUTE_NONE:    // метод не определён
            break;
    }
}
```

## Query-параметры

Параметры query-строки (после `?`) доступны через поле `ctx->request->query_` и функции из `query.h`. Кроме строкового `query_param_char` есть типизированные извлекатели чисел и коллекций:

```c
#include "query.h"

void list_users(httpctx_t* ctx) {
    int ok = 0;

    int page = query_param_int(ctx->request->query_, "page", &ok);
    if (!ok) page = 1; // значение по умолчанию

    const char* q = query_param_char(ctx->request->query_, "q", &ok);
    if (!ok) q = NULL;

    // ...
}
```

::: tip Проверяйте флаг ok для чисел
`0` может быть как валидным значением, так и признаком ошибки (параметра нет или неверный формат). Для числовых параметров проверка `ok` обязательна. Полный список функций (`int`, `double`, массивы, объекты) — в разделе [Query-параметры](/query-params).
:::

## Параметры маршрута

Динамические сегменты URL задаются в `config.json` форматом `{name|pattern}` и попадают в тот же список `query_`:

```json
// config.json
"/api/users/{id|\\d+}": {
    "GET": { "file": "handlers/libapi.so", "function": "get_user" }
}
```

```c
#include "query.h"

void get_user(httpctx_t* ctx) {
    int ok = 0;
    const char* id = query_param_char(ctx->request->query_, "id", &ok);
    // для /api/users/123 здесь id == "123"
    if (!ok) {
        ctx->response->send_default(ctx->response, 400);
        return;
    }
}
```

Подробнее о правилах маршрутизации: [Маршрутизация](/routing).

## Тело запроса

Данные тела извлекаются методами `get_payload*` на объекте запроса. Поддерживаются `multipart/form-data`, `application/x-www-form-urlencoded`, `application/json` и произвольный body.

### JSON

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

### Поле формы

Для `multipart/form-data` и `application/x-www-form-urlencoded` значение поля получается по ключу. Возвращённую строку нужно освободить через `free()`:

```c
void post_form(httpctx_t* ctx) {
    char* email = ctx->request->get_payloadf(ctx->request, "email");
    if (email == NULL) {
        ctx->response->send_data(ctx->response, "Missing field");
        return;
    }

    ctx->response->send_data(ctx->response, email);
    free(email);
}
```

Полный обзор методов (`get_payload`, `get_payloadf`, `get_payload_file*`, `get_payload_jsonf`) и работа с загруженными файлами: [Получение данных от клиента](/payload).

## HTTP-заголовки

`get_header` ищет заголовок по имени (без учёта регистра) и возвращает указатель на узел `http_header_t` либо `NULL`:

```c
typedef struct http_header {
    char*  key;
    char*  value;
    size_t key_length;
    size_t value_length;
    struct http_header* next;
} http_header_t;
```

```c
void handler(httpctx_t* ctx) {
    http_header_t* host = ctx->request->get_header(ctx->request, "Host");
    http_header_t* auth = ctx->request->get_header(ctx->request, "Authorization");

    if (host) {
        printf("Host: %s\n", host->value);
    }

    if (auth) {
        // проверка токена...
    }
}
```

Связанные методы:

| Метод | Описание |
|-------|----------|
| `get_header(name)` | Заголовок по имени (null-terminated) |
| `get_headern(name, length)` | Поиск по имени с явной длиной |
| `add_header(name, value)` | Добавить заголовок |
| `add_headern(name, name_len, value, value_len)` | Добавить с явными длинами |
| `remove_header(name)` | Удалить заголовок |

## Cookie

`get_cookie` возвращает **значение** cookie строкой напрямую (не структуру) либо `NULL`:

```c
void handler(httpctx_t* ctx) {
    const char* token = ctx->request->get_cookie(ctx->request, "session_token");

    if (token == NULL) {
        ctx->response->send_data(ctx->response, "Not authorized");
        return;
    }

    // валидация токена...
}
```

Подробнее об установке и чтении cookie: [Cookie](/cookie).

## user_data

Поле `ctx->user_data` хранит произвольный указатель и удобно для передачи данных между middleware и обработчиком — например, аутентифицированного пользователя. В примере приложения для него есть типизированные помощники:

```c
httpctx_set_user(ctx, user);          // установить
user_t* user = httpctx_get_user(ctx); // получить
```

## База данных

Доступ к БД через `dbquery` с идентификатором хоста. Формат `<драйвер>.<host_id>` выбирает конкретный хост; `<драйвер>` без `host_id` равномерно (round-robin) распределяет запросы по всем хостам этого драйвера:

```c
#include "db.h"

void handler(httpctx_t* ctx) {
    dbresult_t* result = dbquery("postgresql.p1", "SELECT 1", NULL);

    if (!dbresult_ok(result)) {
        ctx->response->send_data(ctx->response, "Database not available");
        dbresult_free(result);
        return;
    }

    // запросы...
    dbresult_free(result);
}
```

Подробнее: [База данных](/db).
