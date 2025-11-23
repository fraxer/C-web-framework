---
outline: deep
description: Извлечение данных из HTTP-запросов в C Web Framework. Работа с multipart/form-data, x-www-form-urlencoded, JSON и загрузка файлов.
---

# Получение данных от клиента

Фреймворк предоставляет методы для извлечения данных из тела HTTP-запросов. Поддерживаются форматы `multipart/form-data`, `application/x-www-form-urlencoded` и `application/json`.

## get_payload

Извлекает всё тело запроса как строку.

```c
#include "http.h"

void post_handler(httpctx_t* ctx) {
    char* payload = ctx->request->get_payload(ctx->request);

    if (!payload) {
        ctx->response->send_data(ctx->response, "Empty payload");
        return;
    }

    ctx->response->send_data(ctx->response, payload);
    free(payload);
}
```

> Возвращает указатель на динамически выделенную память. Обязательно освобождайте память через `free()`.

## get_payloadf

Извлекает значение поля по ключу. Работает с `multipart/form-data` и `application/x-www-form-urlencoded`.

```c
void post_form(httpctx_t* ctx) {
    char* email = ctx->request->get_payloadf(ctx->request, "email");
    char* password = ctx->request->get_payloadf(ctx->request, "password");

    if (!email || !password) {
        ctx->response->send_data(ctx->response, "Missing required fields");
        if (email) free(email);
        if (password) free(password);
        return;
    }

    // Обработка данных...

    free(email);
    free(password);
    ctx->response->send_data(ctx->response, "Form processed");
}
```

## get_payload_file

Извлекает загруженный файл из тела запроса.

```c
void upload_handler(httpctx_t* ctx) {
    file_content_t file = ctx->request->get_payload_file(ctx->request);

    if (!file.ok) {
        ctx->response->send_data(ctx->response, "No file uploaded");
        return;
    }

    // Сохранение файла
    if (!file.save(&file, "uploads", "uploaded_file.txt")) {
        ctx->response->send_data(ctx->response, "Error saving file");
        return;
    }

    // Чтение содержимого файла
    char* content = file.read(&file);
    if (content) {
        ctx->response->send_data(ctx->response, content);
        free(content);
    }
}
```

### Структура file_content_t

| Поле/Метод | Описание |
|------------|----------|
| `ok` | Флаг успешного извлечения |
| `filename` | Имя файла из запроса |
| `save(file, dir, name)` | Сохранение в директорию |
| `read(file)` | Чтение содержимого |

## get_payload_filef

Извлекает файл по ключу из `multipart/form-data`.

```c
void upload_form(httpctx_t* ctx) {
    file_content_t avatar = ctx->request->get_payload_filef(ctx->request, "avatar");

    if (!avatar.ok) {
        ctx->response->send_data(ctx->response, "Avatar not provided");
        return;
    }

    // Сохранение с оригинальным именем
    if (!avatar.save(&avatar, "avatars", avatar.filename)) {
        ctx->response->send_data(ctx->response, "Error saving avatar");
        return;
    }

    ctx->response->send_data(ctx->response, "Avatar uploaded");
}
```

## get_payload_json

Парсит тело запроса как JSON и возвращает документ.

```c
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
    json_token_t* age = json_object_get(root, "age");

    if (name && json_is_string(name)) {
        printf("Name: %s\n", json_string(name));
    }

    if (age && json_is_int(age)) {
        printf("Age: %d\n", json_int(age));
    }

    // Модификация и отправка обратно
    json_object_set(root, "status", json_create_string(doc, "processed"));

    ctx->response->add_header(ctx->response, "Content-Type", "application/json");
    ctx->response->send_data(ctx->response, json_stringify(doc));

    json_free(doc);
}
```

## get_payload_jsonf

Извлекает JSON из поля `multipart/form-data` по ключу.

```c
void multipart_json(httpctx_t* ctx) {
    json_doc_t* metadata = ctx->request->get_payload_jsonf(ctx->request, "metadata");

    if (!json_ok(metadata)) {
        ctx->response->send_data(ctx->response, "Invalid metadata JSON");
        json_free(metadata);
        return;
    }

    json_token_t* root = json_root(metadata);
    json_token_t* title = json_object_get(root, "title");

    if (title && json_is_string(title)) {
        printf("Title: %s\n", json_string(title));
    }

    json_free(metadata);
    ctx->response->send_data(ctx->response, "Metadata processed");
}
```

## Работа со Storage

Сохранение загруженного файла в хранилище:

```c
#include "storage.h"

void upload_to_storage(httpctx_t* ctx) {
    file_content_t file = ctx->request->get_payload_filef(ctx->request, "document");

    if (!file.ok) {
        ctx->response->send_data(ctx->response, "No file");
        return;
    }

    // Сохранение в S3-совместимое хранилище
    if (!storage_file_content_put("remote", &file, "documents/%s", file.filename)) {
        ctx->response->send_data(ctx->response, "Storage error");
        return;
    }

    ctx->response->send_data(ctx->response, "File uploaded to storage");
}
```

## Важно

- Все методы `payload*` возвращают динамически выделенную память
- Обязательно освобождайте память через `free()` или `json_free()`
- Проверяйте результат на `NULL` или `ok` перед использованием
- Максимальный размер тела запроса ограничен параметром `client_max_body_size` в конфигурации
