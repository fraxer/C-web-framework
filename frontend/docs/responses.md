---
outline: deep
description: Формирование HTTP-ответов в C Web Framework. Код состояния, заголовки, отправка данных, JSON, моделей и шаблонов, редиректы, файлы и cookie.
---

# HTTP-ответы

HTTP-ответы формируются через объект `httpctx_t->response` (`httpresponse_t*`). Как и у запроса, методы ответа — это указатели на функции, навешанные на объект, поэтому первым аргументом всегда передаётся сам объект: `ctx->response->send_data(ctx->response, "...")`.

```c
#include "http.h"

void my_handler(httpctx_t* ctx) {
    ctx->response->send_data(ctx->response, "Hello World");
}
```

Ответ содержит код состояния, заголовки и тело. Тело формируется одним из методов `send_*` — до его вызова клиенту ничего не отправляется.

## Код состояния

Поле `status_code` хранит текущий код (по умолчанию `200`). Изменить его можно прямым присваиванием перед отправкой тела:

```c
void handler(httpctx_t* ctx) {
    ctx->response->status_code = 201; // Created
    ctx->response->send_data(ctx->response, "Resource created");
}
```

### Стандартный ответ

`send_default()` отправляет стандартный ответ с указанным кодом и сгенерированной HTML-страницей — удобно для ошибок:

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

Полный список кодов: [HTTP-коды](/http-codes).

## HTTP-заголовки

Заголовки добавляются до отправки тела. `add_header` принимает null-terminated строки, вариант `n` — с явными длинами, а `add_headeru` добавляет заголовок только если он ещё не установлен (уникальный):

```c
void handler(httpctx_t* ctx) {
    ctx->response->add_header(ctx->response, "X-Custom-Header", "value");
    ctx->response->send_data(ctx->response, "ok");
}
```

Связанные методы:

| Метод | Описание |
|-------|----------|
| `add_header(key, value)` | Добавить заголовок (null-terminated) |
| `add_headern(key, key_len, value, value_len)` | Добавить с явными длинами |
| `add_headeru(key, key_len, value, value_len)` | Добавить, только если ещё не установлен |
| `add_content_length(length)` | Добавить `Content-Length` |
| `get_header(key)` | Получить узел заголовка по имени (без учёта регистра) или `NULL` |
| `remove_header(key)` | Удалить заголовок |

::: tip Content-Type задаётся методом отправки
Методы `send_*` сами проставляют `Content-Type`, `Connection` и `Cache-Control` через `add_headeru`. Чтобы переопределить `Content-Type`, добавьте свой заголовок **до** вызова `send_*` — тогда стандартное значение не будет дублироваться.
:::

## Отправка данных

`send_data` отправляет null-terminated строку, `send_datan` — буфер с явной длиной (бинарно-безопасно). Оба выставляют `Content-Type: text/html; charset=utf-8`:

```c
void handler(httpctx_t* ctx) {
    ctx->response->send_data(ctx->response, "Hello World!");
}

void raw(httpctx_t* ctx) {
    const char data[] = { 0x00, 0x01, 0x02, 0x03 };
    ctx->response->send_datan(ctx->response, data, sizeof(data));
}
```

## JSON

`send_json` сериализует готовый `json_doc_t*` и выставляет `Content-Type: application/json`. Документ освобождает caller:

```c
#include "json.h"

void handler(httpctx_t* ctx) {
    json_doc_t* doc = json_init();
    json_token_t* root = json_create_object(doc);

    json_object_set(root, "status", json_create_string(doc, "success"));
    json_object_set(root, "code", json_create_int(doc, 200));

    ctx->response->send_json(ctx->response, doc);

    json_free(doc);
}
```

Подробнее о построении документов: [JSON](/json).

## Модели

Модель ORM сериализуется в JSON напрямую через `send_model`, массив моделей — через `send_models`. Набор отображаемых полей задаётся макросом `display_fields(...)` из `mparams.h`; `NULL` — все поля модели.

```c
#include "user.h"
#include "mparams.h"

void get_user(httpctx_t* ctx) {
    user_t* user = user_get(ctx); // пример получения модели

    if (user == NULL) {
        ctx->response->send_default(ctx->response, 404);
        return;
    }

    // Только указанные поля
    ctx->response->send_model(ctx->response, user,
                              display_fields("id", "email", "name"));
}
```

Массив моделей (например, для списков):

```c
void list_users(httpctx_t* ctx) {
    array_t* users = user_list(ctx); // возвращает array_t*

    if (users == NULL) {
        ctx->response->send_default(ctx->response, 500);
        return;
    }

    ctx->response->send_models(ctx->response, users,
                               display_fields("id", "email", "name"));
}
```

::: tip Макрос display_fields
`display_fields(...)` раскрывается в `char*[]` с завершающим `NULL`, поэтому принимает любое число имён полей: `display_fields("id", "name")` или один `NULL` для всех полей.
:::

Определение моделей и CRUD-обёртки: [Модели](/model).

## Шаблоны

`send_view` рендерит шаблон (view) из именованного хранилища. Первый аргумент — `json_doc_t*` с данными для шаблона (можно `NULL`), затем имя хранилища и путь с поддержкой `printf`-форматирования:

```c
void index(httpctx_t* ctx) {
    json_doc_t* document = json_init();
    json_token_t* root = json_create_object(document);
    json_object_set(root, "title", json_create_string(document, "Главная"));

    ctx->response->send_view(ctx->response, document, "views", "/index.tpl");

    json_free(document);
}
```

О синтаксисе шаблонов: [Представления (views)](/view).

## Редиректы

`redirect` добавляет заголовок `Location` и выставляет код:

```c
void handler(httpctx_t* ctx) {
    ctx->response->redirect(ctx->response, "/new-location", 301); // постоянный
}

void temp(httpctx_t* ctx) {
    ctx->response->redirect(ctx->response, "/temporary", 302); // временный
}
```

Поддерживаются коды `301`, `302`, `307`, `308`. Внешние ссылки (`http://`/`https://`) определяются автоматически; для них добавляется `Connection: Close`.

## Файлы

`send_file` отправляет файл по пути относительно корня сервера, `send_filen` — с явной длиной пути. `Content-Type` определяется по расширению, поддерживается диапазонная передача (Range):

```c
void download(httpctx_t* ctx) {
    ctx->response->send_file(ctx->response, "files/report.pdf");
}
```

`send_filef` читает файл из именованного хранилища с `printf`-форматированием пути (см. `storages` в `config.json`):

```c
void from_storage(httpctx_t* ctx) {
    ctx->response->send_filef(ctx->response, "local", "/uploads/%s", "document.pdf");
}
```

Если файл не найден или недоступен, методы сами отправляют `404`/`403`. Подробнее о хранилищах: [Хранилище](/storage).

## Cookie

`add_cookie` добавляет `Set-Cookie` из структуры `cookie_t`:

```c
typedef struct {
    const char* name;      /* имя (обязательно) */
    const char* value;     /* значение (обязательно) */
    int seconds;           /* время жизни: >0 — Expires, 0 — Max-Age=0 (удаление) */
    const char* path;      /* Path (NULL — пропустить) */
    const char* domain;    /* Domain (NULL — пропустить) */
    int secure;            /* 1 — только HTTPS */
    int http_only;         /* 1 — недоступна из JavaScript */
    const char* same_site; /* "Strict" | "Lax" | "None" (NULL — пропустить) */
} cookie_t;
```

Установка cookie при входе:

```c
void login(httpctx_t* ctx) {
    // ... проверка учётных данных ...

    ctx->response->add_cookie(ctx->response, (cookie_t){
        .name = "session_token",
        .value = "abc123xyz",
        .seconds = 3600,
        .path = "/",
        .domain = ".example.com",
        .secure = 1,
        .http_only = 1,
        .same_site = "Lax"
    });

    ctx->response->send_data(ctx->response, "Logged in");
}
```

Удаление cookie — пустое значение и `seconds = 0` (выставляет `Max-Age=0`):

```c
void logout(httpctx_t* ctx) {
    ctx->response->add_cookie(ctx->response, (cookie_t){
        .name = "session_token",
        .value = "",
        .seconds = 0,
        .path = "/"
    });

    ctx->response->redirect(ctx->response, "/login", 302);
}
```

Чтение cookie из запроса: [Cookie](/cookie).

## CORS

CORS-заголовки добавляются как обычные через `add_header`:

```c
void api_handler(httpctx_t* ctx) {
    ctx->response->add_header(ctx->response, "Access-Control-Allow-Origin", "*");
    ctx->response->add_header(ctx->response, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE");
    ctx->response->add_header(ctx->response, "Access-Control-Allow-Headers", "Content-Type, Authorization");

    if (ctx->request->method == ROUTE_OPTIONS) {
        ctx->response->status_code = 204;
        ctx->response->send_data(ctx->response, "");
        return;
    }

    // основная логика...
}
```

## Важно

- Клиенту ничего не отправляется, пока не вызван один из `send_*` (`send_data`, `send_datan`, `send_json`, `send_model`, `send_models`, `send_view`, `send_file`, `send_filef`, `send_default`) или `redirect`.
- После отправки ответа обработчик должен завершиться (`return`) — повторная отправка даст некорректный ответ.
- `send_default`, `send_json` и `send_model*` сами выставляют `Content-Type`, дублировать его не нужно.
- `send_file`/`send_filef` возвращают стандартный код при отсутствии файла, отдельная проверка не требуется.
