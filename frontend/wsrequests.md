---
outline: deep
title: WebSocket-запросы
description: Обработка WebSocket-запросов в C Web Framework. Формат сообщений, извлечение параметров, методы GET/POST/PATCH/DELETE.
---

# WebSocket-запросы

WebSocket обеспечивает постоянное двунаправленное соединение между клиентом и сервером, позволяя обмениваться данными в реальном времени без накладных расходов HTTP. С помощью его API вы можете отправить сообщение на сервер и получить ответ без выполнения http запроса, причём этот процесс будет событийно-управляемым.

## Запросы

Запросы, сделанные к приложению, представлены в виде структуры `websocketsrequest_t`, которая предоставляет информацию о параметрах WebSocket-запроса - метод, тип, ресурс, данные и т.д.

Формат запроса: `[method] [path] [payload]`.

Метод, путь и полезная нагрузка разделяются одним пробелом, например:

```
GET /
GET /users?id=100
POST /users {"id": 100, "name":"Alex"}
PATCH /users/100 {"name":"Alen"}
DELETE /users/100
```

## Параметры GET запроса

Чтобы получить параметры запроса, используйте контекст `wsctx_t`:

```c
void get(wsctx_t* ctx) {
    const char* data = ctx->request->query(ctx->request, "mydata");

    if (data) {
        ctx->response->send_text(ctx->response, data);
        return;
    }

    ctx->response->send_text(ctx->response, "Data not found");
}
```

## Данные POST и PATCH запросов

Данные, которые были переданы через POST и PATCH отправляются в теле запроса.

Доступ к ним осуществляется через функции:

* `websocketsrequest_payload(protocol)`
* `websocketsrequest_payload_json(protocol)`

```c
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

## Методы запроса

Вы можете получить названия HTTP метода, используемого в текущем запросе, через протокол.

Доступны следующие методы:

* ROUTE_GET
* ROUTE_POST
* ROUTE_PATCH
* ROUTE_DELETE

```c
void get(wsctx_t* ctx) {
    const char* data = ctx->request->method == ROUTE_GET ?
        "This is the get method" :
        "This is the not get method";

    ctx->response->send_text(ctx->response, data);
}
```

## URL запроса

Компонент `request` предоставляет три свойства для работы с url:

* uri - вернет относительный адрес ресурса `/resource.html?id=100&text=hello`.
* path - вернет путь до ресурса без параметров `/resource.html`.
* ext - вернет расширение из названия ресурса `html` или пустую строку.

Также вычисляется длина каждого свойства:

* uri_length
* path_length
* ext_length
