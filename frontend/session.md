---
outline: deep
description: Управление сессиями в C Web Framework. Хранение в файлах или Redis, создание, чтение, обновление и удаление сессий.
---

# Сессии

Сессии позволяют сохранять состояние пользователя между HTTP-запросами. Фреймворк поддерживает два драйвера для хранения сессий: файловая система и Redis.

## Конфигурация

Настройки сессий указываются в файле `config.json`:

```json
{
    "sessions": {
        "driver": "storage",
        "storage_name": "sessions",
        "lifetime": 3600
    }
}
```

**Параметры:**
- `driver` — драйвер хранения: `storage` (файловая система) или `redis`
- `storage_name` — имя хранилища из секции `storages` (для файлового драйвера) или `host_id` Redis-сервера
- `lifetime` — время жизни сессии в секундах

### Пример конфигурации с Redis

```json
{
    "databases": {
        "redis": [
            {
                "host_id": "r1",
                "ip": "127.0.0.1",
                "port": 6379,
                "dbindex": 0
            }
        ]
    },
    "sessions": {
        "driver": "redis",
        "storage_name": "r1",
        "lifetime": 3600
    }
}
```

## API сессий

### Создание сессии

```c
#include "session.h"

char* session_create(const char* data);
```

Создает новую сессию с указанными данными.

**Параметры**\
`data` — строка с данными сессии (обычно JSON).

**Возвращаемое значение**\
Указатель на строку с идентификатором сессии. Необходимо освободить память функцией `free()`.

<br>

### Получение данных сессии

```c
char* session_get(const char* session_id);
```

Получает данные сессии по идентификатору.

**Параметры**\
`session_id` — идентификатор сессии.

**Возвращаемое значение**\
Указатель на строку с данными сессии. Необходимо освободить память функцией `free()`. Возвращает `NULL`, если сессия не найдена.

<br>

### Обновление сессии

```c
int session_update(const char* session_id, const char* data);
```

Обновляет данные существующей сессии.

**Параметры**\
`session_id` — идентификатор сессии.\
`data` — новые данные сессии.

**Возвращаемое значение**\
Ненулевое значение при успехе, ноль при ошибке.

<br>

### Удаление сессии

```c
int session_destroy(const char* session_id);
```

Удаляет сессию.

**Параметры**\
`session_id` — идентификатор сессии.

**Возвращаемое значение**\
Ненулевое значение при успехе, ноль при ошибке.

<br>

### Генерация идентификатора

```c
char* session_create_id();
```

Генерирует уникальный идентификатор сессии.

**Возвращаемое значение**\
Указатель на строку с идентификатором. Необходимо освободить память функцией `free()`.

## Пример использования

```c
#include "http.h"
#include "session.h"
#include "appconfig.h"

void login(httpctx_t* ctx) {
    // Создаем данные сессии
    json_doc_t* doc = json_root_create_object();
    json_token_t* object = json_root(doc);
    json_object_set(object, "user_id", json_create_number(42));
    json_object_set(object, "role", json_create_string("admin"));

    // Создаем сессию
    char* session_id = session_create(json_stringify(doc));
    json_free(doc);

    if (session_id == NULL) {
        ctx->response->send_data(ctx->response, "Session creation failed");
        return;
    }

    // Отправляем session_id в cookie
    ctx->response->add_cookie(ctx->response, (cookie_t){
        .name = "session_id",
        .value = session_id,
        .seconds = appconfig()->sessionconfig.lifetime,
        .path = "/",
        .secure = 1,
        .http_only = 1,
        .same_site = "Lax"
    });

    free(session_id);
    ctx->response->send_data(ctx->response, "Login successful");
}

void profile(httpctx_t* ctx) {
    // Получаем session_id из cookie
    const char* session_id = ctx->request->get_cookie(ctx->request, "session_id");
    if (session_id == NULL) {
        ctx->response->send_data(ctx->response, "Not authenticated");
        return;
    }

    // Получаем данные сессии
    char* session_data = session_get(session_id);
    if (session_data == NULL) {
        ctx->response->send_data(ctx->response, "Session expired");
        return;
    }

    // Парсим данные
    json_doc_t* doc = json_parse(session_data);
    free(session_data);

    if (doc == NULL) {
        ctx->response->send_data(ctx->response, "Invalid session data");
        return;
    }

    json_token_t* root = json_root(doc);
    json_token_t* user_id_token = json_object_get(root, "user_id");

    int ok = 0;
    int user_id = json_int(user_id_token, &ok);
    json_free(doc);

    char response[64];
    snprintf(response, sizeof(response), "User ID: %d", user_id);
    ctx->response->send_data(ctx->response, response);
}

void logout(httpctx_t* ctx) {
    const char* session_id = ctx->request->get_cookie(ctx->request, "session_id");
    if (session_id != NULL) {
        session_destroy(session_id);
    }

    // Удаляем cookie
    ctx->response->add_cookie(ctx->response, (cookie_t){
        .name = "session_id",
        .value = "",
        .seconds = 0,
        .path = "/"
    });

    ctx->response->send_data(ctx->response, "Logged out");
}
```

## Автоматическая очистка

```c
void session_remove_expired(void);
```

Удаляет все просроченные сессии. Рекомендуется вызывать периодически для очистки устаревших данных.
