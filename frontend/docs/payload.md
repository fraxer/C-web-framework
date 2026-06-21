---
outline: deep
description: Извлечение данных из HTTP-запросов в C Web Framework. Работа с multipart/form-data, x-www-form-urlencoded, JSON и загрузка файлов.
---

# Получение данных от клиента

Фреймворк предоставляет методы для извлечения данных из тела HTTP-запроса. Тело сохраняется во временный spool-файл и парсится лениво — при первом обращении через методы `get_payload*`. Поддерживаются форматы `multipart/form-data`, `application/x-www-form-urlencoded`, `application/json` и произвольный бинарный/текстовый body.

Методы навешаны на объект запроса и вызываются через `ctx->request-><метод>(ctx->request, ...)`.

## Обзор методов

| Метод | Возвращает | Назначение | Форматы тела |
|-------|-----------|-----------|--------------|
| `get_payload` | `char*` | Всё тело запроса одной строкой | `PLAIN`, `MULTIPART` |
| `get_payloadf` | `char*` | Значение поля по ключу | `MULTIPART`, `URLENCODED` |
| `get_payload_file` | `file_content_t` | Файл из тела запроса (первая часть) | `MULTIPART`, `PLAIN` |
| `get_payload_filef` | `file_content_t` | Файл по ключу | `MULTIPART` |
| `get_payload_json` | `json_doc_t*` | Тело, распарсенное как JSON | любой текстовый body |
| `get_payload_jsonf` | `json_doc_t*` | JSON из поля по ключу | `MULTIPART` |

::: tip Внутренний тип body
При чтении заголовка `Content-Type` фреймворк определяет тип тела (`http_payload_type_e`): `PLAIN` — произвольный body (включая `application/json`), `MULTIPART` — `multipart/form-data`, `URLENCODED` — `application/x-www-form-urlencoded`, `NONE` — тела нет. Выбор подходящего метода зависит от этого типа.
:::

## get_payload

`char* (*get_payload)(httprequest_t* request);`

Извлекает всё тело запроса как строку. Эквивалентен вызову `get_payloadf(request, NULL)` — возвращает первую часть (`PLAIN`) или весь body `MULTIPART`. Для `URLENCODED` возвращает `NULL` (используйте `get_payloadf` с ключом).

```c
#include "http.h"

void post_handler(httpctx_t* ctx) {
    char* payload = ctx->request->get_payload(ctx->request);

    if (payload == NULL) {
        ctx->response->send_data(ctx->response, "Empty payload");
        return;
    }

    ctx->response->send_data(ctx->response, payload);

    free(payload);
}
```

> Возвращает указатель на динамически выделенную память. Обязательно освобождайте через `free()`.

## get_payloadf

`char* (*get_payloadf)(httprequest_t* request, const char* field);`

Извлекает значение поля по ключу. Работает с `multipart/form-data` и `application/x-www-form-urlencoded`.

```c
#include "http.h"

void post_form(httpctx_t* ctx) {
    char* email = ctx->request->get_payloadf(ctx->request, "email");
    char* password = ctx->request->get_payloadf(ctx->request, "password");

    if (email == NULL || password == NULL) {
        ctx->response->send_data(ctx->response, "Missing required fields");
        goto failed;
    }

    // Обработка данных...

    ctx->response->send_data(ctx->response, "Form processed");

    failed:

    if (email) free(email);
    if (password) free(password);
}
```

## get_payload_file

`file_content_t (*get_payload_file)(httprequest_t* request);`

Извлекает загруженный файл из тела запроса — первую часть `MULTIPART` (или весь body для `PLAIN`). Эквивалентен `get_payload_filef(request, NULL)`. Возвращает lightweight-структуру `file_content_t`, которая **не копирует** данные, а ссылается на смещение во внутреннем spool-файле. Копирование происходит только в `make_file()` или `content()`.

```c
#include "http.h"

void upload_handler(httpctx_t* ctx) {
    file_content_t file = ctx->request->get_payload_file(ctx->request);

    if (!file.ok) {
        ctx->response->send_data(ctx->response, "No file uploaded");
        return;
    }

    // make_file(dir, name) копирует данные на диск и возвращает открытый file_t.
    // name = NULL сохраняет под оригинальным именем file.filename.
    file_t saved = file.make_file(&file, "uploads", "uploaded_file.txt");
    if (!saved.ok) {
        ctx->response->send_data(ctx->response, "Error saving file");
        return;
    }

    // content() читает байты в выделенный буфер (освобождается через free).
    char* content = file.content(&file);
    if (content) {
        ctx->response->send_data(ctx->response, content);
        free(content);
    }

    saved.close(&saved);
}
```

### Структура `file_content_t`

| Поле / Метод | Тип | Описание |
|--------------|-----|----------|
| `ok` | `int` | Флаг успешного извлечения (`1` = данные валидны) |
| `fd` | `int` | Файловый дескриптор внутреннего spool-файла |
| `offset` | `off_t` | Смещение, с которого начинаются данные файла |
| `size` | `size_t` | Размер данных в байтах |
| `filename` | `char[NAME_MAX]` | Имя файла, переданное клиентом |
| `set_filename(name)` | `int` | Задать имя (`1` — успех) |
| `make_file(dir, name)` | `file_t` | Сохранить в директорию `dir` (`name = NULL` → оригинальное имя) |
| `make_tmpfile(dir)` | `file_t` | Сохранить во временный файл в `dir` (удаляется при `close()`) |
| `content()` | `char*` | Прочитать байты в память (освобождается через `free`) |

## get_payload_filef

`file_content_t (*get_payload_filef)(httprequest_t* request, const char* field);`

Извлекает файл по ключу из `multipart/form-data`. Если поле найдено, но не содержит файла (нет `filename`), `ok` будет `0`.

```c
#include "http.h"

void upload_form(httpctx_t* ctx) {
    file_content_t avatar = ctx->request->get_payload_filef(ctx->request, "avatar");

    if (!avatar.ok) {
        ctx->response->send_data(ctx->response, "Avatar not provided");
        return;
    }

    // NULL — сохранить под оригинальным именем из запроса.
    file_t saved = avatar.make_file(&avatar, "avatars", NULL);
    if (!saved.ok) {
        ctx->response->send_data(ctx->response, "Error saving avatar");
        return;
    }

    saved.close(&saved);

    ctx->response->send_data(ctx->response, "Avatar uploaded");
}
```

### Структура `file_t`

Возвращается из `make_file()` / `make_tmpfile()`. Основные методы:

| Метод | Возвращает | Описание |
|-------|-----------|----------|
| `ok` / `tmp` / `fd` | поля | `ok` — успех; `tmp=1` — временный файл (удалится при `close`); `fd` — дескриптор |
| `size` / `mtime` / `name` | поля | Размер, время модификации, имя файла |
| `content()` | `char*` | Прочитать содержимое (через `free`) |
| `set_content(data, size)` | `int` | Перезаписать содержимое |
| `append_content(data, size)` | `int` | Дописать в конец |
| `truncate(offset)` | `int` | Обрезать до указанного размера |
| `close()` | `int` | Закрыть (и удалить, если `tmp=1`) |

## get_payload_json

`json_doc_t* (*get_payload_json)(httprequest_t* request);`

Парсит всё тело запроса как JSON и возвращает документ. Эквивалентен `get_payload_jsonf(request, NULL)`. Возвращает `NULL` при ошибке парсинга.

```c
#include "http.h"
#include "json.h"

void post_json(httpctx_t* ctx) {
    json_doc_t* doc = ctx->request->get_payload_json(ctx->request);

    if (!json_ok(doc)) {
        ctx->response->send_data(ctx->response, json_error(doc));
        json_free(doc);
        return;
    }

    json_token_t* root = json_root(doc);

    if (!json_is_object(root)) {
        ctx->response->send_data(ctx->response, "Expected JSON object");
        json_free(doc);
        return;
    }

    // Извлечение полей
    json_token_t* name = json_object_get(root, "name");
    json_token_t* age  = json_object_get(root, "age");

    if (name && json_is_string(name)) {
        printf("Name: %s\n", json_string(name));
    }

    if (age && json_is_number(age)) {
        int ok = 0;
        printf("Age: %d\n", json_int(age, &ok));
    }

    // Модификация и отправка обратно
    json_object_set(root, "status", json_create_string("processed"));

    ctx->response->add_header(ctx->response, "Content-Type", "application/json");
    ctx->response->send_data(ctx->response, json_stringify(doc));

    json_free(doc);
}
```

## get_payload_jsonf

`json_doc_t* (*get_payload_jsonf)(httprequest_t* request, const char* field);`

Извлекает JSON из поля `multipart/form-data` по ключу и парсит его. Удобно для отправки файлов вместе со структурированными метаданными.

```c
#include "http.h"
#include "json.h"

void multipart_json(httpctx_t* ctx) {
    json_doc_t* metadata = ctx->request->get_payload_jsonf(ctx->request, "metadata");

    if (!json_ok(metadata)) {
        ctx->response->send_data(ctx->response, "Invalid metadata JSON");
        json_free(metadata);
        return;
    }

    json_token_t* root  = json_root(metadata);
    json_token_t* title = json_object_get(root, "title");

    if (title && json_is_string(title)) {
        printf("Title: %s\n", json_string(title));
    }

    json_free(metadata);

    ctx->response->send_data(ctx->response, "Metadata processed");
}
```

## Сохранение в Storage

Загруженный файл можно напрямую отправить в подключённое хранилище (FS или S3-совместимое), минуя ручное копирование:

```c
#include "http.h"
#include "storage.h"

void upload_to_storage(httpctx_t* ctx) {
    file_content_t file = ctx->request->get_payload_filef(ctx->request, "document");

    if (!file.ok) {
        ctx->response->send_data(ctx->response, "No file");
        return;
    }

    // Сохранение в именованное хранилище "remote" (см. секцию storages в config.json).
    if (!storage_file_content_put("remote", &file, "documents/%s", file.filename)) {
        ctx->response->send_data(ctx->response, "Storage error");
        return;
    }

    ctx->response->send_data(ctx->response, "File uploaded to storage");
}
```

## Управление памятью

- `get_payload` / `get_payloadf` возвращают динамически выделенную `char*` — освобождайте через `free()`.
- `get_payload_json` / `get_payload_jsonf` возвращают `json_doc_t*` — освобождайте через `json_free()`.
- `file_content_t` из `get_payload_file*` **не владеет** данными — копирование происходит только при вызове `make_file()` / `content()`. Буфер из `content()` освобождается через `free()`, а `file_t` из `make_file()` закрывается через `close()`.
- Всегда проверяйте результат на `NULL` (строки/JSON) или поле `ok` (`file_content_t` / `file_t`) перед использованием.
- Максимальный размер тела запроса ограничен параметром `client_max_body_size` в секции `main` конфигурации.
