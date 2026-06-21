---
outline: deep
title: WebSocket-запросы
description: Обработка WebSocket-запросов в C Web Framework. Контекст wsctx_t, ресурсный протокол, метод, URL, query-параметры и тело кадра.
---

# WebSocket-запросы

WebSocket обеспечивает постоянное двунаправленное соединение между клиентом и сервером, позволяя обмениваться данными в реальном времени без накладных расходов HTTP. С помощью его API вы можете отправить сообщение на сервер и получить ответ без выполнения http запроса, причём этот процесс будет событийно-управляемым.

## Контекст запроса

WebSocket-запросы обрабатываются через контекст `wsctx_t`. Он содержит указатели на объекты запроса и ответа, а также слот для произвольных пользовательских данных.

```c
typedef struct wsctx {
    websocketsrequest_t* request;   // объект запроса
    websocketsresponse_t* response; // объект ответа
    void* user_data;                // произвольные данные (например, аутентифицированный пользователь)
} wsctx_t;
```

Объект `websocketsrequest_t` хранит служебные поля кадра (тип, фрагментацию, сжатие) и ссылку на **протокол**. Именно протокол разбирает HTTP-подобное сообщение и хранит метод, путь, query-параметры и тело:

```c
typedef struct websocketsrequest {
    request_t base;                  // reset / free
    websockets_datatype_e type;      // тип кадра: TEXT, BINARY, PING, PONG, CLOSE
    websockets_protocol_t* protocol; // протокол разбора и маршрутизации
    int can_reset;
    int fragmented;                  // сообщение разнесено по нескольким кадрам
    int compressed;                 // сжато permessage-deflate
    connection_t* connection;        // соединение, которому принадлежит запрос
} websocketsrequest_t;
```

Доступ к данным запроса ведётся через протокол. Фреймворк поставляет два варианта: `websockets_protocol_resource_t` (HTTP-подобная маршрутизация `METHOD /path?query DATA`) и упрощённый `websockets_protocol_default_t` (только тело кадра, без адреса и метода). Ниже речь о ресурсном протоколе — именно он используется в `config.json` для маршрутов.

## Базовая структура обработчика

```c
#include "websockets.h"

void my_handler(wsctx_t* ctx) {
    ctx->response->send_text(ctx->response, "Hello World");
}
```

Методы запроса и ответа — это указатели на функции, навешанные на объект. Первый аргумент всегда сам объект: `ctx->response->send_text(ctx->response, ...)`.

## Формат запроса

Сообщения ресурсного протокола имеют вид `[method] [path] [payload]`. Метод, путь и полезная нагрузка разделяются одним пробелом, например:

```
GET /
GET /users?id=100
POST /users {"id": 100, "name":"Alex"}
PATCH /users/100 {"name":"Alen"}
DELETE /users/100
```

Распознаваемые методы — `GET`, `POST`, `PATCH`, `DELETE`. Сообщение с другим методом отклоняется.

## HTTP-метод

Текущий метод доступен через поле `protocol->method` после приведения протокола к `websockets_protocol_resource_t` и представлен значениями enum `route_methods_e` (`core/src/route/route.h`):

```c
#include "websockets.h"

void handler(wsctx_t* ctx) {
    websockets_protocol_resource_t* protocol = (websockets_protocol_resource_t*)ctx->request->protocol;

    switch (protocol->method) {
        case ROUTE_GET:    // GET
        case ROUTE_POST:   // POST
        case ROUTE_PATCH:  // PATCH
        case ROUTE_DELETE: // DELETE
            break;
        case ROUTE_NONE:   // метод не определён
            break;
    }
}
```

::: tip Метод привязан к протоколу
Поле `method` есть у ресурсного протокола, а не у самого объекта запроса. У `websockets_protocol_default_t` метода и пути нет — он работает только с телом кадра.
:::

## URL запроса

Ресурсный протокол разбирает адрес на составляющие:

| Поле | Тип | Описание |
|------|-----|----------|
| `uri` | `char*` | Сырой URI с query-строкой: `/users?id=100` |
| `path` | `char*` | URL-декодированный путь без query-строки: `/users` |
| `uri_length` | `size_t` | Длина `uri` |
| `path_length` | `size_t` | Длина `path` |
| `query_` | `query_t*` | Связный список query-параметров и параметров маршрута |

```c
void handler(wsctx_t* ctx) {
    websockets_protocol_resource_t* protocol = (websockets_protocol_resource_t*)ctx->request->protocol;

    ctx->response->send_text(ctx->response, protocol->path);
}
```

::: tip Готовые длины
Поля `uri` и `path` нуль-терминированы, поэтому `strlen()` работает, но `uri_length` и `path_length` берутся напрямую из парсера и не требуют прохода по строке.
:::

## Query-параметры

Параметры query-строки (после `?`) и параметры маршрута попадают в общий список `protocol->query_`. Самый короткий способ прочитать значение — метод протокола `get_query`, который возвращает строку или `NULL`, если параметр отсутствует:

```c
#include "websockets.h"

void get(wsctx_t* ctx) {
    websockets_protocol_resource_t* protocol = (websockets_protocol_resource_t*)ctx->request->protocol;
    const char* data = protocol->get_query(protocol, "mydata");

    if (data) {
        ctx->response->send_text(ctx->response, data);
        return;
    }

    ctx->response->send_text(ctx->response, "Data not found");
}
```

Для типобезопасного извлечения — чисел, дробных значений, массивов — используйте функции из `query.h` напрямую со списком `protocol->query_`, как в HTTP-обработчиках. Подробнее: [Query-параметры](/query-params).

## Тело запроса

Тело передаётся с методами `POST` и `PATCH`. Извлечение выполняется функциями `websocketsrequest_payload*` или одноимёнными методами протокола. Полный обзор (включая работу с файлами и JSON) — в разделе [Получение данных от клиента](/wspayload).

```c
#include "websockets.h"

void post(wsctx_t* ctx) {
    char* payload = websocketsrequest_payload(ctx->request->protocol);

    if (!payload) {
        ctx->response->send_text(ctx->response, "Payload not found");
        return;
    }

    ctx->response->send_text(ctx->response, payload);

    free(payload);
}
```

| Способ | Возвращает |
|--------|-----------|
| `websocketsrequest_payload` / `protocol->get_payload` | `char*` — всё тело строкой |
| `websocketsrequest_payload_file` / `protocol->get_payload_file` | `file_content_t` — тело как файловый дескриптор |
| `websocketsrequest_payload_json` / `protocol->get_payload_json` | `json_doc_t*` — тело, распарсенное как JSON |

## Соединение

Поле `ctx->request->connection` указывает на нижележащее соединение. Оно нужно для работы с каналами широковещания — например, чтобы добавить соединение в канал или разослать сообщение подписчикам. Подробнее: [Широковещание](/wsbroadcast).
