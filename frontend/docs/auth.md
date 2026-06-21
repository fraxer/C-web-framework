---
outline: deep
description: Аутентификация в C Web Framework. Хеширование паролей, сессии, RBAC и защита маршрутов.
---

# Аутентификация

Фреймворк предоставляет инструменты для реализации аутентификации пользователей: хеширование паролей (PBKDF2-HMAC-SHA256), управление сессиями, валидация учётных данных и ролевой доступ (RBAC).

## Хеширование паролей

Для хеширования используется PBKDF2-HMAC-SHA256. Соль и хеш хранятся в одном поле модели `user` в формате `iterations$salt$hash` (значения salt и hash — в шестнадцатеричной кодировке). Параметры по умолчанию определены в `auth.h`:

```c
#define SALT_SIZE 16      // размер соли в байтах
#define HASH_SIZE 32      // размер хеша в байтах
#define ITERATIONS 140000 // количество итераций PBKDF2
```

### Генерация секрета

```c
#include "auth.h"

str_t* generate_secret(const char* password);
```

Генерирует безопасный секрет из пароля: создаёт случайную соль, хеширует пароль и собирает строку `iterations$salt$hash`.

**Параметры**\
`password` — пароль пользователя.

**Возвращаемое значение**\
Строка в формате `iterations$salt$hash`. Необходимо освободить функцией `str_free()`. `NULL` при ошибке.

<br>

### Сборка секрета из готовых значений

```c
str_t* create_secret(int iterations, const char* hash_hex, const char* salt_hex);
```

Собирает секрет из числа итераций, хеша и соли (оба значения — hex-строки), разделяя их символом `$`. Используется внутри `generate_secret()`, но доступна и для самостоятельного вызова — например, при смене числа итераций.

**Параметры**\
`iterations` — количество итераций PBKDF2.\
`hash_hex` — хеш пароля в hex.\
`salt_hex` — соль в hex.

**Возвращаемое значение**\
Строка секрета. Необходимо освободить функцией `str_free()`. `NULL` при ошибке.

<br>

### Хеширование с существующей солью

```c
int password_hash(const char* password, unsigned char* salt, int salt_size, unsigned char* hash);
```

Хеширует пароль с указанной солью и числом итераций `ITERATIONS`.

**Параметры**\
`password` — пароль.\
`salt` — соль.\
`salt_size` — размер соли.\
`hash` — буфер размером `HASH_SIZE` для записи хеша.

**Возвращаемое значение**\
1 при успехе, 0 при ошибке.

<br>

### Генерация соли

```c
int generate_salt(unsigned char* salt, int size);
```

Генерирует криптографически безопасную случайную соль (`RAND_bytes` из OpenSSL).

**Параметры**\
`salt` — буфер для записи соли.\
`size` — размер соли.

**Возвращаемое значение**\
1 при успехе, 0 при ошибке.

## Аутентификация пользователя

### По email и паролю

```c
user_t* authenticate(const char* email, const char* password);
```

Проверяет email и пароль, загружает пользователя из БД, извлекает соль из секрета, хеширует пароль и сверяет с хранимым хешем.

**Параметры**\
`email` — email пользователя.\
`password` — пароль.

**Возвращаемое значение**\
Указатель на модель пользователя при успехе, `NULL` при ошибке (невалидные данные, пользователь не найден, несовпадение хеша). Пользователя необходимо освободить через `user_free()`.

<br>

### По cookie `token`

```c
user_t* authenticate_by_cookie(httpctx_t* ctx);
```

Аутентифицирует пользователя по cookie `token` из запроса — выполняет поиск пользователя по токену. Подходит для schemes на основе долгоживущих токенов доступа (например, «запомнить меня»), альтернативных сессиям.

**Параметры**\
`ctx` — контекст HTTP-запроса.

**Возвращаемое значение**\
Указатель на модель пользователя при успехе, `NULL` при ошибке.

::: tip Два способа аутентификации
Фреймворк поддерживает два независимых механизма:

- **Сессионный** — `middleware_http_auth(ctx)` проверяет cookie `session_id`, читает данные сессии через `session_get()` и загружает пользователя в контекст (`httpctx_get_user(ctx)`). Используется в примерах ниже (вход, защищённый маршрут).
- **Токенный** — `authenticate_by_cookie(ctx)` ищет пользователя напрямую по cookie `token` без участия хранилища сессий.
:::

## Примеры использования

### Регистрация пользователя

```c
#include "http.h"
#include "auth.h"
#include "user.h"

void registration(httpctx_t* ctx) {
    // Получаем данные из тела запроса
    const char* email = ctx->request->get_payloadf(ctx->request, "email");
    if (email == NULL) {
        ctx->response->status_code = 400;
        ctx->response->send_data(ctx->response, "Email is required");
        return;
    }

    const char* password = ctx->request->get_payloadf(ctx->request, "password");
    if (password == NULL) {
        ctx->response->status_code = 400;
        ctx->response->send_data(ctx->response, "Password is required");
        return;
    }

    const char* name = ctx->request->get_payloadf(ctx->request, "name");

    // Валидация
    if (!validate_email(email)) {
        ctx->response->status_code = 400;
        ctx->response->send_data(ctx->response, "Invalid email");
        return;
    }

    if (!validate_password(password)) {
        ctx->response->status_code = 400;
        ctx->response->send_data(ctx->response, "Invalid password");
        return;
    }

    // Генерируем секрет (хеш пароля)
    str_t* secret = generate_secret(password);
    if (secret == NULL) {
        ctx->response->status_code = 500;
        ctx->response->send_data(ctx->response, "Error generating password hash");
        return;
    }

    // Создаём пользователя
    user_t* user = user_instance();
    user_set_email(user, email);
    user_set_name(user, name ? name : "");
    user_set_secret(user, str_get(secret));

    str_free(secret);

    if (!user_create(user)) {
        user_free(user);
        ctx->response->status_code = 500;
        ctx->response->send_data(ctx->response, "Error creating user");
        return;
    }

    ctx->response->send_data(ctx->response, "Registration successful");
    user_free(user);
}
```

### Вход в систему

```c
void login(httpctx_t* ctx) {
    const char* email = ctx->request->get_payloadf(ctx->request, "email");
    const char* password = ctx->request->get_payloadf(ctx->request, "password");

    if (email == NULL || password == NULL) {
        ctx->response->status_code = 400;
        ctx->response->send_data(ctx->response, "Email and password are required");
        return;
    }

    // Аутентификация
    user_t* user = authenticate(email, password);
    if (user == NULL) {
        ctx->response->status_code = 401;
        ctx->response->send_data(ctx->response, "Invalid credentials");
        return;
    }

    // Создаём сессию: session_create(<имя>, <данные>, <ttl в секундах>)
    json_doc_t* doc = json_root_create_object();
    json_token_t* root = json_root(doc);
    json_object_set(root, "user_id", json_create_number(user_id(user)));

    char* session_id = session_create("backend", json_stringify(doc), 86400);
    json_free(doc);

    if (session_id == NULL) {
        user_free(user);
        ctx->response->status_code = 500;
        ctx->response->send_data(ctx->response, "Error creating session");
        return;
    }

    // Устанавливаем cookie
    ctx->response->add_cookie(ctx->response, (cookie_t){
        .name = "session_id",
        .value = session_id,
        .seconds = 86400,  // 24 часа
        .path = "/",
        .secure = 1,
        .http_only = 1,
        .same_site = "Lax"
    });

    free(session_id);
    user_free(user);

    ctx->response->send_data(ctx->response, "Login successful");
}
```

### Выход из системы

```c
void logout(httpctx_t* ctx) {
    const char* session_id = ctx->request->get_cookie(ctx->request, "session_id");

    if (session_id != NULL) {
        // session_destroy(<имя>, <id сессии>)
        session_destroy("backend", session_id);
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

### Защищённый маршрут

```c
#include "http.h"
#include "auth.h"
#include "user.h"

void profile(httpctx_t* ctx) {
    // Проверяем авторизацию через middleware.
    // При успехе пользователь доступен через httpctx_get_user(ctx).
    middleware(middleware_http_auth(ctx));

    user_t* user = httpctx_get_user(ctx);

    // send_model сериализует модель в JSON.
    // display_fields(...) перечисляет поля, которые попадут в ответ,
    // поэтому secret гарантированно исключается.
    ctx->response->send_model(ctx->response, user, display_fields("id", "email", "name"));
}
```

## Ролевой доступ (RBAC)

Модели `role`, `permission`, `role_permission` и `user_role` уже входят в состав приложения (`app/models/`). Ниже — схема связей и примеры проверок на уровне SQL.

### Структура таблиц

```sql
-- Роли
CREATE TABLE role (
    id SERIAL PRIMARY KEY,
    name VARCHAR(64) UNIQUE NOT NULL
);

-- Разрешения
CREATE TABLE permission (
    id SERIAL PRIMARY KEY,
    name VARCHAR(64) UNIQUE NOT NULL
);

-- Связь ролей и разрешений
CREATE TABLE role_permission (
    role_id INTEGER REFERENCES role(id),
    permission_id INTEGER REFERENCES permission(id),
    PRIMARY KEY (role_id, permission_id)
);

-- Связь пользователей и ролей
CREATE TABLE user_role (
    user_id INTEGER REFERENCES "user"(id),
    role_id INTEGER REFERENCES role(id),
    PRIMARY KEY (user_id, role_id)
);
```

### Проверка роли

Параметры запроса передаются по имени (`:name`) и собираются макросами `mparam_*`.

```c
int user_has_role(user_t* user, const char* role_name) {
    array_t* params = array_create();
    mparams_fill_array(params,
        mparam_int(user_id, user_id(user)),
        mparam_varchar(role_name, role_name)
    );

    dbresult_t* result = dbquery(
        "postgresql.p1",
        "SELECT 1 FROM user_role ur "
        "JOIN role r ON r.id = ur.role_id "
        "WHERE ur.user_id = :user_id AND r.name = :role_name",
        params
    );

    array_free(params);

    int has_role = (result != NULL && dbresult_query_rows(result) > 0);
    dbresult_free(result);

    return has_role;
}
```

### Проверка разрешения

```c
int user_has_permission(user_t* user, const char* permission_name) {
    array_t* params = array_create();
    mparams_fill_array(params,
        mparam_int(user_id, user_id(user)),
        mparam_varchar(permission_name, permission_name)
    );

    dbresult_t* result = dbquery(
        "postgresql.p1",
        "SELECT 1 FROM user_role ur "
        "JOIN role_permission rp ON rp.role_id = ur.role_id "
        "JOIN permission p ON p.id = rp.permission_id "
        "WHERE ur.user_id = :user_id AND p.name = :permission_name",
        params
    );

    array_free(params);

    int has_permission = (result != NULL && dbresult_query_rows(result) > 0);
    dbresult_free(result);

    return has_permission;
}
```

### Middleware для проверки роли

```c
int middleware_require_role(httpctx_t* ctx, const char* role) {
    user_t* user = httpctx_get_user(ctx);
    if (user == NULL) {
        ctx->response->send_default(ctx->response, 401);
        return 0;
    }

    if (!user_has_role(user, role)) {
        ctx->response->send_default(ctx->response, 403);
        return 0;
    }

    return 1;
}

// Использование
void admin_panel(httpctx_t* ctx) {
    middleware(
        middleware_http_auth(ctx),
        middleware_require_role(ctx, "admin")
    );

    ctx->response->send_data(ctx->response, "Welcome to admin panel");
}
```

### Middleware для проверки разрешения

```c
int middleware_require_permission(httpctx_t* ctx, const char* permission) {
    user_t* user = httpctx_get_user(ctx);
    if (user == NULL) {
        ctx->response->send_default(ctx->response, 401);
        return 0;
    }

    if (!user_has_permission(user, permission)) {
        ctx->response->send_default(ctx->response, 403);
        return 0;
    }

    return 1;
}

// Использование
void delete_post(httpctx_t* ctx) {
    middleware(
        middleware_http_auth(ctx),
        middleware_require_permission(ctx, "posts.delete")
    );

    // Удаление поста...
}
```

## Валидация

### Валидация email

```c
int validate_email(const char* email);
```

Проверяет корректность email-адреса: длина до 254 символов, ровно один `@`, корректные локальную часть (до 64 символов, допустимые символы RFC 5322, без точек в начале/конце и подряд) и домен (валидные символы, хотя бы одна точка, корректный TLD из букв длиной от 2 символов).

**Возвращаемое значение**\
1 — адрес корректен, 0 — нет.

### Валидация пароля

```c
int validate_password(const char* password);
```

Проверяет соответствие пароля требованиям безопасности:

- длина от 8 до 128 символов;
- минимум по одной заглавной и строчной букве, цифре и спецсимволу;
- отсутствие подряд повторяющихся символов;
- отсутствие распространённых шаблонов (`123`, `abc`, `qwe`, `password`, …) и клавиатурных последовательностей (`qwerty`, `asdfgh`, …) — в том числе в обратном порядке.

**Возвращаемое значение**\
1 — пароль надёжный, 0 — нет.

## Константы

```c
#define SALT_SIZE 16      // размер соли в байтах
#define HASH_SIZE 32      // размер хеша в байтах
#define ITERATIONS 140000 // количество итераций PBKDF2
```
