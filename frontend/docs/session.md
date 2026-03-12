---
outline: deep
description: Управление сессиями в C Web Framework. Хранение в файлах, Redis или базе данных, создание, чтение, обновление и удаление сессий.
---

# Сессии

Сессии позволяют сохранять состояние пользователя между HTTP-запросами. Фреймворк поддерживает три драйвера для хранения сессий: файловая система, Redis и база данных.

## Конфигурация

Настройки сессий указываются в файле `config.json`. Секция `sessions` представляет собой именованную карту конфигураций сессий, где каждый ключ — уникальное имя, используемое для обращения к конфигурации в коде:

```json
{
    "sessions": {
        "default": {
            "driver": "filesystem",
            "storage_name": "sessions",
            "secret": "my-secret-passphrase"
        }
    }
}
```

**Параметры:**
- `driver` — драйвер хранения: `filesystem`, `redis` или `database`
- `storage_name` — имя хранилища из секции `storages` (для драйвера `filesystem`)
- `host_id` — идентификатор хоста базы данных/Redis-сервера (для драйверов `redis` и `database`)
- `secret` — секретная фраза для шифрования данных сессии (обязательное поле)

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
        "default": {
            "driver": "redis",
            "host_id": "r1",
            "secret": "my-secret-passphrase"
        }
    }
}
```

### Пример конфигурации с базой данных

```json
{
    "databases": {
        "postgresql": [
            {
                "host_id": "p1",
                "ip": "127.0.0.1",
                "port": 5432,
                "dbname": "mydb",
                "user": "root",
                "password": ""
            }
        ]
    },
    "sessions": {
        "default": {
            "driver": "database",
            "host_id": "p1",
            "secret": "my-secret-passphrase"
        }
    }
}
```

## API сессий

### Создание сессии

```c
#include "session.h"

char* session_create(const char* key, const char* data, long duration);
```

Создает новую сессию с указанными данными.

**Параметры**\
`key` — имя конфигурации сессии из `config.json`.\
`data` — строка с данными сессии (обычно JSON).\
`duration` — время жизни сессии в секундах.

**Возвращаемое значение**\
Указатель на строку с идентификатором сессии. Необходимо освободить память функцией `free()`. Возвращает `NULL` при ошибке.

<br>

### Получение данных сессии

```c
char* session_get(const char* key, const char* session_id);
```

Получает данные сессии по идентификатору.

**Параметры**\
`key` — имя конфигурации сессии из `config.json`.\
`session_id` — идентификатор сессии.

**Возвращаемое значение**\
Указатель на строку с данными сессии. Необходимо освободить память функцией `free()`. Возвращает `NULL`, если сессия не найдена.

<br>

### Обновление сессии

```c
int session_update(const char* key, const char* session_id, const char* data);
```

Обновляет данные существующей сессии.

**Параметры**\
`key` — имя конфигурации сессии из `config.json`.\
`session_id` — идентификатор сессии.\
`data` — новые данные сессии.

**Возвращаемое значение**\
Ненулевое значение при успехе, ноль при ошибке.

<br>

### Удаление сессии

```c
int session_destroy(const char* key, const char* session_id);
```

Удаляет сессию.

**Параметры**\
`key` — имя конфигурации сессии из `config.json`.\
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

void login(httpctx_t* ctx) {
    // Создаем данные сессии
    json_doc_t* doc = json_root_create_object();
    json_token_t* object = json_root(doc);
    json_object_set(object, "user_id", json_create_number(42));
    json_object_set(object, "role", json_create_string("admin"));

    // Создаем сессию с временем жизни 3600 секунд
    char* session_id = session_create("default", json_stringify(doc), 3600);
    json_free(doc);

    if (session_id == NULL) {
        ctx->response->send_data(ctx->response, "Session creation failed");
        return;
    }

    // Отправляем session_id в cookie
    ctx->response->add_cookie(ctx->response, (cookie_t){
        .name = "session_id",
        .value = session_id,
        .seconds = 3600,
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
    char* session_data = session_get("default", session_id);
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
        session_destroy("default", session_id);
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
void session_remove_expired(const char* key);
```

Удаляет все просроченные сессии для указанной конфигурации. Рекомендуется вызывать периодически для очистки устаревших данных.

**Параметры**\
`key` — имя конфигурации сессии из `config.json`.
