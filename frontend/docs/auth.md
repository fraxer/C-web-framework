---
outline: deep
description: Аутентификация в C Web Framework. Хеширование паролей, сессии, RBAC и защита маршрутов.
---

# Аутентификация

Фреймворк предоставляет инструменты для реализации аутентификации пользователей: хеширование паролей (PBKDF2-SHA256), управление сессиями и ролевой доступ (RBAC).

## Хеширование паролей

### Генерация секрета

```c
#include "auth.h"

str_t* generate_secret(const char* password);
```

Генерирует безопасный секрет из пароля, используя PBKDF2-SHA256 с случайной солью.

**Параметры**\
`password` — пароль пользователя.

**Возвращаемое значение**\
Строка в формате `iterations$salt$hash`. Необходимо освободить функцией `str_free()`.

<br>

### Хеширование с существующей солью

```c
int password_hash(const char* password, unsigned char* salt, int salt_size, unsigned char* hash);
```

Хеширует пароль с указанной солью.

**Параметры**\
`password` — пароль.\
`salt` — соль.\
`salt_size` — размер соли.\
`hash` — буфер для записи хеша.

**Возвращаемое значение**\
1 при успехе, 0 при ошибке.

<br>

### Генерация соли

```c
int generate_salt(unsigned char* salt, int size);
```

Генерирует криптографически безопасную случайную соль.

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

Аутентифицирует пользователя по email и паролю.

**Параметры**\
`email` — email пользователя.\
`password` — пароль.

**Возвращаемое значение**\
Указатель на модель пользователя при успехе, `NULL` при ошибке.

<br>

### По cookie

```c
user_t* authenticate_by_cookie(httpctx_t* ctx);
```

Аутентифицирует пользователя по cookie из запроса.

**Параметры**\
`ctx` — контекст HTTP-запроса.

**Возвращаемое значение**\
Указатель на модель пользователя при успехе, `NULL` при ошибке.

## Примеры использования

### Регистрация пользователя

```c
#include "http.h"
#include "auth.h"
#include "user.h"

void registration(httpctx_t* ctx) {
    int ok = 0;

    // Получаем данные из формы
    const char* email = ctx->request->get_payload_text(ctx->request, "email", &ok);
    if (!ok || email == NULL) {
        ctx->response->status_code = 400;
        ctx->response->send_data(ctx->response, "Email is required");
        return;
    }

    const char* password = ctx->request->get_payload_text(ctx->request, "password", &ok);
    if (!ok || password == NULL) {
        ctx->response->status_code = 400;
        ctx->response->send_data(ctx->response, "Password is required");
        return;
    }

    const char* name = ctx->request->get_payload_text(ctx->request, "name", &ok);

    // Валидация
    if (!validate_email(email)) {
        ctx->response->status_code = 400;
        ctx->response->send_data(ctx->response, "Invalid email");
        return;
    }

    if (!validate_password(password)) {
        ctx->response->status_code = 400;
        ctx->response->send_data(ctx->response, "Password must be at least 8 characters");
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
    int ok = 0;

    const char* email = ctx->request->get_payload_text(ctx->request, "email", &ok);
    const char* password = ctx->request->get_payload_text(ctx->request, "password", &ok);

    if (!email || !password) {
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

    // Создаём сессию
    json_doc_t* doc = json_root_create_object();
    json_token_t* root = json_root(doc);
    json_object_set(root, "user_id", json_create_number(user_id(user)));

    char* session_id = session_create(json_stringify(doc));
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
        .same_site = "Strict"
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

### Защищённый маршрут

```c
void profile(httpctx_t* ctx) {
    // Проверяем авторизацию через middleware
    middleware(middleware_http_auth(ctx));

    // Получаем пользователя из контекста
    user_t* user = httpctx_get_user(ctx);

    json_doc_t* doc = json_root_create_object();
    json_token_t* root = json_root(doc);
    json_object_set(root, "id", json_create_number(user_id(user)));
    json_object_set(root, "email", json_create_string(user_email(user)));
    json_object_set(root, "name", json_create_string(user_name(user)));

    ctx->response->header_add(ctx->response, "Content-Type", "application/json");
    ctx->response->send_data(ctx->response, json_stringify(doc));

    json_free(doc);
}
```

## Ролевой доступ (RBAC)

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

```c
int user_has_role(user_t* user, const char* role_name) {
    array_t* params = array_create();
    mparams_fill_array(params,
        mparam_int(user_id, user_id(user)),
        mparam_varchar(role_name, role_name)
    );

    dbresult_t* result = db_query(
        "postgresql.p1",
        "SELECT 1 FROM user_role ur "
        "JOIN role r ON r.id = ur.role_id "
        "WHERE ur.user_id = $1 AND r.name = $2",
        params
    );

    array_free(params);

    int has_role = (result != NULL && db_result_row_count(result) > 0);
    db_result_free(result);

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

    dbresult_t* result = db_query(
        "postgresql.p1",
        "SELECT 1 FROM user_role ur "
        "JOIN role_permission rp ON rp.role_id = ur.role_id "
        "JOIN permission p ON p.id = rp.permission_id "
        "WHERE ur.user_id = $1 AND p.name = $2",
        params
    );

    array_free(params);

    int has_permission = (result != NULL && db_result_row_count(result) > 0);
    db_result_free(result);

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

Проверяет корректность email-адреса.

### Валидация пароля

```c
int validate_password(const char* password);
```

Проверяет соответствие пароля требованиям безопасности.

## Константы

```c
#define SALT_SIZE 16    // Размер соли в байтах
#define HASH_SIZE 32    // Размер хеша в байтах
#define ITERATIONS 10000 // Количество итераций PBKDF2
```
