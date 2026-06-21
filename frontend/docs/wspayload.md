---
outline: deep
title: WebSocket Payload
description: Извлечение данных из WebSocket-сообщений в C Web Framework. Методы payload, payload_file и payload_json для чтения тела входящего кадра.
---

# Получение данных от клиента

Фреймворк предоставляет функции для извлечения тела входящего WebSocket-кадра. Тело сообщения сохраняется во временный файл по мере приёма кадра и читается лениво — только при вызове одной из функций ниже. Формат данных произвольный: текст (включая JSON) или бинарные байты.

Все функции принимают протокол соединения `ctx->request->protocol` и работают одинаково для текстовых и бинарных кадров.

## Обзор функций

| Функция | Возвращает | Назначение |
|---------|-----------|-----------|
| `websocketsrequest_payload` | `char*` | Всё тело кадра одной строкой |
| `websocketsrequest_payload_file` | `file_content_t` | Тело кадра как файловый дескриптор (без копирования) |
| `websocketsrequest_payload_json` | `json_doc_t*` | Тело кадра, распарсенное как JSON |

::: tip Два способа вызова
Каждая функция доступна в двух равнозначных формах: как свободная функция `websocketsrequest_payload(protocol)` и как метод протокола `protocol->get_payload(protocol)`. Методы протокола — это те же функции, привязанные к объекту, поэтому выбирайте тот вариант, который ближе по стилю. Свободная функция принимает базовый `websockets_protocol_t*` и не требует приведения типа.
:::

## websocketsrequest_payload

`char* websocketsrequest_payload(websockets_protocol_t* protocol);`

Читает всё тело кадра в нуль-терминированную строку. Удобно для текстовых сообщений и сырых бинарных данных, которые нужно получить одним блоком. Возвращает `NULL`, если тело пустое.

```c
#include "websockets.h"

void echo(wsctx_t* ctx) {
    char* payload = websocketsrequest_payload(ctx->request->protocol);

    if (payload == NULL) {
        ctx->response->send_text(ctx->response, "Empty payload");
        return;
    }

    ctx->response->send_text(ctx->response, payload);

    free(payload);
}
```

> Возвращает указатель на динамически выделенную память. Обязательно освобождайте через `free()`.

## websocketsrequest_payload_file

`file_content_t websocketsrequest_payload_file(websockets_protocol_t* protocol);`

Возвращает тело кадра как lightweight-структуру `file_content_t`, которая **не копирует** данные, а ссылается на смещение во внутреннем временном файле. Это эффективный способ работать с большими бинарными сообщениями: копирование происходит только при явном вызове `content()` или `make_file()`. Поле `ok` равно `0`, если тело пустое.

```c
#include "websockets.h"

void upload(wsctx_t* ctx) {
    file_content_t payload = websocketsrequest_payload_file(ctx->request->protocol);

    if (!payload.ok) {
        ctx->response->send_text(ctx->response, "Empty payload");
        return;
    }

    // content() читает байты в выделенный буфер (освобождается через free).
    char* data = payload.content(&payload);
    if (data) {
        ctx->response->send_binaryn(ctx->response, data, payload.size);
        free(data);
    }
}
```

### Структура `file_content_t`

| Поле / Метод | Тип | Описание |
|--------------|-----|----------|
| `ok` | `int` | Флаг успешного извлечения (`1` = данные валидны) |
| `fd` | `int` | Файловый дескриптор внутреннего временного файла |
| `offset` | `off_t` | Сметение, с которого начинаются данные |
| `size` | `size_t` | Размер данных в байтах |
| `filename` | `char[NAME_MAX]` | Имя файла; для WebSocket всегда `"tmpfile"` (кадр не несёт имени) |
| `make_file(dir, name)` | `file_t` | Сохранить в директорию `dir` (`name = NULL` → `"tmpfile"`) |
| `make_tmpfile(dir)` | `file_t` | Сохранить во временный файл в `dir` (удаляется при `close()`) |
| `content()` | `char*` | Прочитать байты в память (освобождается через `free`) |

::: warning Имя файла
В отличие от HTTP `multipart/form-data`, WebSocket-кадр не передаёт имени файла, поэтому `filename` всегда равен `"tmpfile"`. При сохранении передавайте имя явно через второй аргумент `make_file(dir, name)`.
:::

## websocketsrequest_payload_json

`json_doc_t* websocketsrequest_payload_json(websockets_protocol_t* protocol);`

Парсит тело кадра как JSON и возвращает документ. Возвращает `NULL` при ошибке парсинга или пустом теле. Эквивалентен последовательности `websocketsrequest_payload` → `json_parse`.

```c
#include "websockets.h"
#include "json.h"

void post(wsctx_t* ctx) {
    json_doc_t* document = websocketsrequest_payload_json(ctx->request->protocol);

    if (!json_ok(document)) {
        ctx->response->send_text(ctx->response, json_error(document));
        goto failed;
    }

    json_token_t* object = json_root(document);
    if (!json_is_object(object)) {
        ctx->response->send_text(ctx->response, "is not object");
        goto failed;
    }

    json_object_set(object, "mykey", json_create_string(document, "Hello"));

    ctx->response->send_text(ctx->response, json_stringify(document));

    failed:

    json_free(document);
}
```

## Доступ через протокол

Вместо свободных функций можно вызывать те же методы через протокол соединения. Этот стиль используется в обработчиках, которые уже получили протокол для других операций (например, `get_query`). Понадобится приведение к конкретному типу протокола — `websockets_protocol_resource_t` или `websockets_protocol_default_t`.

```c
#include "websockets.h"
#include "json.h"

void echo(wsctx_t* ctx) {
    websockets_protocol_resource_t* protocol = (websockets_protocol_resource_t*)ctx->request->protocol;

    // Сначала пробуем JSON...
    json_doc_t* document = protocol->get_payload_json(protocol);
    if (document != NULL) {
        ctx->response->send_text(ctx->response, json_stringify(document));
        json_free(document);
        return;
    }

    // ...иначе отдаём как сырую строку
    char* data = protocol->get_payload(protocol);
    if (data) {
        ctx->response->send_text(ctx->response, data);
        free(data);
        return;
    }

    ctx->response->send_text(ctx->response, "done");
}
```

| Метод протокола | Эквивалентная функция |
|-----------------|----------------------|
| `protocol->get_payload(protocol)` | `websocketsrequest_payload` |
| `protocol->get_payload_file(protocol)` | `websocketsrequest_payload_file` |
| `protocol->get_payload_json(protocol)` | `websocketsrequest_payload_json` |

## Управление памятью

- `websocketsrequest_payload` возвращает динамически выделенную `char*` — освобождайте через `free()`.
- `websocketsrequest_payload_json` возвращает `json_doc_t*` — освобождайте через `json_free()`.
- `file_content_t` из `websocketsrequest_payload_file` **не владеет** данными — копирование происходит только при вызове `content()` / `make_file()`. Буфер из `content()` освобождается через `free()`, а `file_t` из `make_file()` закрывается через `close()`.
- Всегда проверяйте результат на `NULL` (строка/JSON) или поле `ok` (`file_content_t`) перед использованием.
- Тело каждого кадра живёт только на время обработки запроса — не сохраняйте указатели на него между вызовами обработчика.
