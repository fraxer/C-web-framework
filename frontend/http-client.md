---
outline: deep
description: HTTP-клиент в C Web Framework. Отправка GET, POST запросов, работа с заголовками, формами и JSON.
---

# HTTP-клиент

Фреймворк включает встроенный HTTP-клиент для выполнения исходящих HTTP-запросов к внешним сервисам.

## Инициализация

```c
#include "httpclient.h"

httpclient_t* httpclient_init(int method, const char* url, int timeout);
```

Создаёт новый HTTP-клиент.

**Параметры**\
`method` — HTTP-метод (`ROUTE_GET`, `ROUTE_POST`, `ROUTE_PUT`, `ROUTE_DELETE` и др.).\
`url` — URL для запроса.\
`timeout` — таймаут в секундах.

**Возвращаемое значение**\
Указатель на клиент или `NULL` при ошибке.

## Отправка запроса

```c
httpresponse_t* client->send(httpclient_t* client);
```

Выполняет HTTP-запрос.

**Возвращаемое значение**\
Указатель на ответ или `NULL` при ошибке соединения.

## Освобождение ресурсов

```c
client->free(httpclient_t* client);
```

Освобождает память клиента и связанных объектов.

## Примеры использования

### GET-запрос

```c
#include "http.h"
#include "httpclient.h"

void fetch_data(httpctx_t* ctx) {
    const char* url = "https://api.example.com/data";
    const int timeout = 5;

    httpclient_t* client = httpclient_init(ROUTE_GET, url, timeout);
    if (client == NULL) {
        ctx->response->status_code = 500;
        ctx->response->send_data(ctx->response, "Failed to create client");
        return;
    }

    // Добавляем заголовки
    httprequest_t* req = client->request;
    req->add_header(req, "Accept", "application/json");
    req->add_header(req, "User-Agent", "C-Web-Framework/1.0");

    // Отправляем запрос
    httpresponse_t* res = client->send(client);
    if (res == NULL) {
        ctx->response->status_code = 502;
        ctx->response->send_data(ctx->response, "Connection failed");
        client->free(client);
        return;
    }

    // Проверяем статус
    if (res->status_code != 200) {
        ctx->response->status_code = res->status_code;
        ctx->response->send_data(ctx->response, "External API error");
        client->free(client);
        return;
    }

    // Получаем тело ответа
    char* body = res->get_payload(res);
    if (body == NULL) {
        ctx->response->status_code = 500;
        ctx->response->send_data(ctx->response, "Failed to read response");
        client->free(client);
        return;
    }

    // Отправляем ответ клиенту
    ctx->response->header_add(ctx->response, "Content-Type", "application/json");
    ctx->response->send_data(ctx->response, body);

    free(body);
    client->free(client);
}
```

### POST с URL-encoded данными

```c
void post_urlencoded(httpctx_t* ctx) {
    const char* url = "https://api.example.com/login";
    httpclient_t* client = httpclient_init(ROUTE_POST, url, 5);

    if (client == NULL) {
        ctx->response->send_data(ctx->response, "Client error");
        return;
    }

    httprequest_t* req = client->request;

    // Добавляем параметры формы
    req->append_urlencoded(req, "username", "john");
    req->append_urlencoded(req, "password", "secret123");

    httpresponse_t* res = client->send(client);
    if (res == NULL || res->status_code != 200) {
        ctx->response->send_data(ctx->response, "Request failed");
        client->free(client);
        return;
    }

    char* body = res->get_payload(res);
    ctx->response->send_data(ctx->response, body);

    free(body);
    client->free(client);
}
```

### POST с JSON-телом

```c
void post_json(httpctx_t* ctx) {
    const char* url = "https://api.example.com/users";
    httpclient_t* client = httpclient_init(ROUTE_POST, url, 5);

    if (client == NULL) {
        ctx->response->send_data(ctx->response, "Client error");
        return;
    }

    httprequest_t* req = client->request;

    // Создаём JSON-документ
    json_doc_t* doc = json_root_create_object();
    json_token_t* root = json_root(doc);
    json_object_set(root, "name", json_create_string("John Doe"));
    json_object_set(root, "email", json_create_string("john@example.com"));
    json_object_set(root, "age", json_create_number(30));

    // Устанавливаем JSON как тело запроса
    req->set_payload_json(req, doc);
    json_free(doc);

    httpresponse_t* res = client->send(client);
    if (res == NULL) {
        ctx->response->send_data(ctx->response, "Connection failed");
        client->free(client);
        return;
    }

    char* body = res->get_payload(res);

    ctx->response->status_code = res->status_code;
    ctx->response->header_add(ctx->response, "Content-Type", "application/json");
    ctx->response->send_data(ctx->response, body ? body : "{}");

    if (body) free(body);
    client->free(client);
}
```

### POST с multipart/form-data

```c
void post_formdata(httpctx_t* ctx) {
    const char* url = "https://api.example.com/upload";
    httpclient_t* client = httpclient_init(ROUTE_POST, url, 10);

    if (client == NULL) {
        ctx->response->send_data(ctx->response, "Client error");
        return;
    }

    httprequest_t* req = client->request;

    // Текстовое поле
    req->append_formdata_text(req, "description", "My file description");

    // Сырые данные с указанием типа
    req->append_formdata_raw(req, "metadata", "{\"type\":\"image\"}", "application/json");

    // JSON-объект
    json_doc_t* doc = json_root_create_object();
    json_token_t* root = json_root(doc);
    json_object_set(root, "tags", json_create_string("photo,vacation"));
    req->append_formdata_json(req, "options", doc);
    json_free(doc);

    // Файл из хранилища
    file_t file = storage_file_get("local", "uploads/image.png");
    if (file.ok) {
        req->append_formdata_file(req, "file", &file);
        file.close(&file);
    }

    httpresponse_t* res = client->send(client);
    if (res == NULL) {
        ctx->response->send_data(ctx->response, "Upload failed");
        client->free(client);
        return;
    }

    char* body = res->get_payload(res);
    ctx->response->send_data(ctx->response, body ? body : "Success");

    if (body) free(body);
    client->free(client);
}
```

### Запрос с авторизацией

```c
void authorized_request(httpctx_t* ctx) {
    const char* url = "https://api.example.com/protected";
    httpclient_t* client = httpclient_init(ROUTE_GET, url, 5);

    if (client == NULL) {
        ctx->response->send_data(ctx->response, "Client error");
        return;
    }

    httprequest_t* req = client->request;

    // Bearer токен
    req->add_header(req, "Authorization", "Bearer your-jwt-token-here");

    // Или Basic авторизация
    // req->add_header(req, "Authorization", "Basic dXNlcjpwYXNz");

    httpresponse_t* res = client->send(client);

    if (res == NULL) {
        ctx->response->send_data(ctx->response, "Connection failed");
        client->free(client);
        return;
    }

    if (res->status_code == 401) {
        ctx->response->status_code = 401;
        ctx->response->send_data(ctx->response, "Unauthorized");
        client->free(client);
        return;
    }

    char* body = res->get_payload(res);
    ctx->response->send_data(ctx->response, body);

    free(body);
    client->free(client);
}
```

### Обработка gzip-ответа

```c
void fetch_compressed(httpctx_t* ctx) {
    const char* url = "https://api.example.com/large-data";
    httpclient_t* client = httpclient_init(ROUTE_GET, url, 10);

    if (client == NULL) {
        ctx->response->send_data(ctx->response, "Client error");
        return;
    }

    httprequest_t* req = client->request;

    // Запрашиваем сжатый ответ
    req->add_header(req, "Accept-Encoding", "gzip");

    httpresponse_t* res = client->send(client);
    if (res == NULL || res->status_code != 200) {
        ctx->response->send_data(ctx->response, "Request failed");
        client->free(client);
        return;
    }

    // get_payload автоматически распаковывает gzip
    char* body = res->get_payload(res);
    ctx->response->send_data(ctx->response, body);

    free(body);
    client->free(client);
}
```

## Доступные методы

| Константа | HTTP-метод |
|-----------|------------|
| `ROUTE_GET` | GET |
| `ROUTE_POST` | POST |
| `ROUTE_PUT` | PUT |
| `ROUTE_PATCH` | PATCH |
| `ROUTE_DELETE` | DELETE |
| `ROUTE_HEAD` | HEAD |
| `ROUTE_OPTIONS` | OPTIONS |

## Методы запроса

```c
// Добавление заголовка
req->add_header(req, "Header-Name", "value");

// URL-encoded данные
req->append_urlencoded(req, "key", "value");

// Multipart form-data
req->append_formdata_text(req, "field", "text value");
req->append_formdata_raw(req, "field", "raw data", "content/type");
req->append_formdata_json(req, "field", json_doc);
req->append_formdata_file(req, "field", &file);

// JSON-тело
req->set_payload_json(req, json_doc);
```

## Методы ответа

```c
// Статус ответа
int status = res->status_code;

// Тело ответа (необходимо освободить через free())
char* body = res->get_payload(res);
```
