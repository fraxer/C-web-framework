---
outline: deep
description: HTTP-клиент в C Web Framework. Отправка GET, POST запросов, работа с заголовками, формами, JSON, TLS и перенаправлениями.
---

# HTTP-клиент

Фреймворк включает встроенный синхронный HTTP-клиент для выполнения исходящих запросов к внешним сервисам. Он поддерживает HTTPS (TLS 1.2+), автоматическое следование перенаправлениям, объединение соединений в пул (keep-alive) и прозрачно обрабатывает запросы к собственным доменам сервера без сетевого цикла (self-invocation).

## Инициализация

```c
#include "httpclient.h"

httpclient_t* httpclient_init(route_methods_e method, const char* url, int timeout);
```

Создаёт новый HTTP-клиент, парсит URL и подготавливает объекты запроса и ответа.

**Параметры**\
`method` — HTTP-метод (`ROUTE_GET`, `ROUTE_POST`, `ROUTE_PUT`, `ROUTE_DELETE`, `ROUTE_PATCH`, `ROUTE_HEAD`, `ROUTE_OPTIONS`).\
`url` — абсолютный URL (`http://` или `https://`); хосты с национальными символами (IDN) поддерживаются и приводятся к punycode автоматически.\
`timeout` — таймаут операции чтения/записи сокета в секундах. При `timeout <= 0` используется значение по умолчанию — `10` секунд.

**Возвращаемое значение**\
Указатель на клиент или `NULL` при ошибке (невалидный URL, нехватка памяти, сбой инициализации OpenSSL).

::: tip Поведение по умолчанию
- Схема `https://` включает TLS; проверка сертификата узла **включена** и используются системные доверенные CA.
- Заголовок `Host` и `Content-Length` выставляются автоматически — задавать их вручную не нужно.
:::

::: warning Таймаут задаётся только при инициализации
Таймаут нельзя изменить после создания клиента: поле `set_timeout` объявлено в структуре, но не подключено. Нужное значение передавайте аргументом `timeout` в `httpclient_init()`.
:::

## Конфигурация клиента

```c
void client->set_method(httpclient_t* client, route_methods_e method);
int  client->set_url(httpclient_t* client, const char* url);
void client->set_verify(httpclient_t* client, int verify, const char* ca_path);
```

| Метод | Описание |
|-------|----------|
| `set_method` | Изменяет HTTP-метод запроса. |
| `set_url` | Заново парсит URL и заменяет схему, хост и порт. Возвращает `1` при успехе, `0` при ошибке парсинга. |
| `set_verify` | Настраивает проверку TLS-сертификата узла. |

### Проверка TLS-сертификата

По умолчанию `verify_peer = 1`: для `https://`-запросов сертификат узла проверяется по доверенным CA. Метод `set_verify` позволяет переопределить это поведение:

```c
// Использовать собственный набор доверенных сертификатов (CA-бандл)
client->set_verify(client, 1, "/etc/ssl/certs/custom-ca-bundle.pem");

// Вернуться к системным путям CA
client->set_verify(client, 1, NULL);

// Отключить проверку (небезопасно — только для тестов/внутренних сервисов)
client->set_verify(client, 0, NULL);
```

## Отправка запроса

```c
httpresponse_t* client->send(httpclient_t* client);
```

Выполняет HTTP-запрос. Метод **всегда возвращает указатель на объект ответа** (`client->response`), он никогда не равен `NULL`. При ошибке соединения, сбое TLS или превышении лимита перенаправлений в ответе выставляется `status_code = 500`.

```c
httpresponse_t* res = client->send(client);

if (res->status_code != 200) {
    // обработать ошибку внешнего сервиса
}
```

### Следование перенаправлениям

Клиент автоматически следует ответам `301` и `302`, читая заголовок `Location`. Для не-безопасных методов (`POST`, `PUT`, `DELETE`, …) метод переключается на `GET`, а тело запроса удаляется (RFC 7231). Максимум выполняется до 10 переходов — при превышении возвращается ответ с `status_code = 500`.

## Чтение ответа

```c
// HTTP-статус ответа
int status = res->status_code;

// Тело ответа как строка (выделена malloc — освободите через free()).
// При наличии Content-Encoding: gzip распаковывается автоматически.
char* body = res->get_payload(res);

// Тело ответа как JSON-документ (освободите через json_free())
json_doc_t* doc = res->get_payload_json(res);

// Заголовок ответа по имени (без копирования)
http_header_t* loc = res->get_header(res, "Content-Type");
```

## Освобождение ресурсов

```c
void client->free(httpclient_t* client);
```

Освобождает память клиента и связанных объектов (запрос, ответ, SSL-контекст, буферы). Безопасно вызывать с `NULL`. Вызывайте на всех путях выхода из обработчика.

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
        ctx->response->send_data(ctx->response, httpresponse_status_string(500));
        return;
    }

    httprequest_t* req = client->request;
    req->add_header(req, "Accept", "application/json");
    req->add_header(req, "User-Agent", "C-Web-Framework/1.0");

    httpresponse_t* res = client->send(client);
    if (res->status_code != 200) {
        ctx->response->status_code = res->status_code;
        ctx->response->send_data(ctx->response, "External API error");
        client->free(client);
        return;
    }

    char* body = res->get_payload(res);

    ctx->response->add_header(ctx->response, "Content-Type", "application/json");
    ctx->response->send_data(ctx->response, body ? body : "");

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
        ctx->response->send_data(ctx->response, httpresponse_status_string(500));
        return;
    }

    httprequest_t* req = client->request;
    req->append_urlencoded(req, "username", "john");
    req->append_urlencoded(req, "password", "secret123");

    httpresponse_t* res = client->send(client);
    if (res->status_code != 200) {
        ctx->response->status_code = res->status_code;
        ctx->response->send_data(ctx->response, "Request failed");
        client->free(client);
        return;
    }

    char* body = res->get_payload(res);
    ctx->response->send_data(ctx->response, body ? body : "");

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
        ctx->response->send_data(ctx->response, httpresponse_status_string(500));
        return;
    }

    json_doc_t* doc = json_root_create_object();
    json_token_t* root = json_root(doc);
    json_object_set(root, "name", json_create_string("John Doe"));
    json_object_set(root, "email", json_create_string("john@example.com"));
    json_object_set(root, "age", json_create_number(30));

    httprequest_t* req = client->request;
    req->set_payload_json(req, doc);
    json_free(doc);

    httpresponse_t* res = client->send(client);
    ctx->response->status_code = res->status_code;
    ctx->response->add_header(ctx->response, "Content-Type", "application/json");

    char* body = res->get_payload(res);
    ctx->response->send_data(ctx->response, body ? body : "{}");

    free(body);
    client->free(client);
}
```

### Произвольное тело (raw / text)

```c
void post_raw(httpctx_t* ctx) {
    httpclient_t* client = httpclient_init(ROUTE_POST, "https://api.example.com/echo", 5);
    if (client == NULL) {
        ctx->response->send_data(ctx->response, httpresponse_status_string(500));
        return;
    }

    httprequest_t* req = client->request;

    // Сырые байты с явным Content-Type и длиной
    req->set_payload_raw(req, "application/octet-stream", 5, "hello");

    // Или просто текст (Content-Type: text/plain)
    // req->set_payload_text(req, "hello");

    httpresponse_t* res = client->send(client);
    char* body = res->get_payload(res);
    ctx->response->status_code = res->status_code;
    ctx->response->send_data(ctx->response, body ? body : "");

    free(body);
    client->free(client);
}
```

### POST с multipart/form-data

```c
void post_formdata(httpctx_t* ctx) {
    const char* url = "https://api.example.com/upload";
    httpclient_t* client = httpclient_init(ROUTE_POST, url, 10);
    if (client == NULL) {
        ctx->response->send_data(ctx->response, httpresponse_status_string(500));
        return;
    }

    httprequest_t* req = client->request;

    // Текстовое поле
    req->append_formdata_text(req, "description", "My file description");

    // Сырые данные с указанием типа
    req->append_formdata_raw(req, "metadata", "application/json", "{\"type\":\"image\"}");

    // JSON-объект
    json_doc_t* doc = json_root_create_object();
    json_token_t* root = json_root(doc);
    json_object_set(root, "tags", json_create_string("photo,vacation"));
    req->append_formdata_json(req, "options", doc);
    json_free(doc);

    // Файл из именованного хранилища (file_t)
    file_t file = storage_file_get("local", "uploads/image.png");
    if (file.ok) {
        req->append_formdata_file(req, "file", &file);
        file.close(&file);
    }

    // Или файл напрямую по пути в файловой системе
    // req->append_formdata_filepath(req, "file", "/var/data/uploads/image.png");

    httpresponse_t* res = client->send(client);
    char* body = res->get_payload(res);
    ctx->response->status_code = res->status_code;
    ctx->response->send_data(ctx->response, body ? body : "Success");

    free(body);
    client->free(client);
}
```

### Запрос с авторизацией

```c
void authorized_request(httpctx_t* ctx) {
    const char* url = "https://api.example.com/protected";
    httpclient_t* client = httpclient_init(ROUTE_GET, url, 5);
    if (client == NULL) {
        ctx->response->send_data(ctx->response, httpresponse_status_string(500));
        return;
    }

    httprequest_t* req = client->request;

    // Bearer-токен
    req->add_header(req, "Authorization", "Bearer your-jwt-token-here");

    // Или Basic-авторизация
    // req->add_header(req, "Authorization", "Basic dXNlcjpwYXNz");

    httpresponse_t* res = client->send(client);
    if (res->status_code == 401) {
        ctx->response->status_code = 401;
        ctx->response->send_data(ctx->response, "Unauthorized");
        client->free(client);
        return;
    }

    char* body = res->get_payload(res);
    ctx->response->send_data(ctx->response, body ? body : "");

    free(body);
    client->free(client);
}
```

### HTTPS с собственной проверкой сертификата

```c
void fetch_secure(httpctx_t* ctx) {
    httpclient_t* client = httpclient_init(ROUTE_GET, "https://internal.example.com/health", 5);
    if (client == NULL) {
        ctx->response->send_data(ctx->response, httpresponse_status_string(500));
        return;
    }

    // Доверять только корпоративному CA-бандлу
    client->set_verify(client, 1, "/etc/ssl/certs/corporate-ca.pem");

    httpresponse_t* res = client->send(client);
    ctx->response->status_code = res->status_code;

    char* body = res->get_payload(res);
    ctx->response->send_data(ctx->response, body ? body : "");

    free(body);
    client->free(client);
}
```

### Обработка gzip-ответа

```c
void fetch_compressed(httpctx_t* ctx) {
    httpclient_t* client = httpclient_init(ROUTE_GET, "https://api.example.com/large-data", 10);
    if (client == NULL) {
        ctx->response->send_data(ctx->response, httpresponse_status_string(500));
        return;
    }

    httprequest_t* req = client->request;
    req->add_header(req, "Accept-Encoding", "gzip");

    httpresponse_t* res = client->send(client);
    if (res->status_code != 200) {
        ctx->response->status_code = res->status_code;
        ctx->response->send_data(ctx->response, "Request failed");
        client->free(client);
        return;
    }

    // get_payload автоматически распаковывает gzip
    char* body = res->get_payload(res);
    ctx->response->send_data(ctx->response, body ? body : "");

    free(body);
    client->free(client);
}
```

## Поведение клиента

- **Заголовки запроса**: `Host` и `Content-Length` проставляются автоматически. Свои заголовки добавляйте через `req->add_header()`.
- **TLS**: для `https://` проверка сертификата включена по умолчанию (минимальная версия TLS 1.2). См. [`set_verify`](#проверка-tls-сертификата).
- **Перенаправления**: `301`/`302` обрабатываются автоматически, до 10 переходов.
- **Сжатие**: gzip-ответы распаковываются прозрачно при вызове `get_payload()`.
- **Keep-alive**: соединения с `Connection: keep-alive` возвращаются в пул и переиспользуются для последующих запросов к тому же хосту/порту.
- **Self-invocation**: если URL указывает на домен, обслуживаемый этим же сервером, запрос выполняется внутренней диспетчеризацией маршрута (вместо сетевого цикла), включая прохождение middleware.

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

## Методы клиента

```c
// Отправка запроса (всегда возвращает объект ответа; 500 при ошибке)
httpresponse_t* res = client->send(client);

// Переконфигурация до отправки
client->set_method(client, ROUTE_POST);
client->set_url(client, "https://api.example.com/v2/data");
client->set_verify(client, 1, "/path/to/ca.pem");

// Освобождение
client->free(client);
```

## Методы запроса

```c
httprequest_t* req = client->request;

// Заголовки
req->add_header(req, "Header-Name", "value");
req->add_headern(req, "Header-Name", name_len, "value", value_len);
req->remove_header(req, "Header-Name");

// URL-encoded форма
req->append_urlencoded(req, "key", "value");

// Multipart form-data
req->append_formdata_text(req, "field", "text value");
req->append_formdata_raw(req, "field", "content/type", "raw data");
req->append_formdata_json(req, "field", json_doc);
req->append_formdata_file(req, "field", &file);          // file_t
req->append_formdata_filepath(req, "field", "/path/file"); // путь в ФС

// Тело запроса
req->set_payload_text(req, "plain text");
req->set_payload_raw(req, "content/type", length, "raw data");
req->set_payload_json(req, json_doc);
req->set_payload_file(req, &file);                        // file_t
req->set_payload_filepath(req, "/path/file");             // путь в ФС
```

## Методы ответа

```c
httpresponse_t* res = client->response;

// Статус ответа
int status = res->status_code;

// Тело как строка (malloc — освободить через free(); gzip распаковывается)
char* body = res->get_payload(res);

// Тело как JSON (освободить через json_free())
json_doc_t* doc = res->get_payload_json(res);

// Заголовок ответа по имени
http_header_t* header = res->get_header(res, "Content-Type");
```
