---
outline: deep
description: Работа с базами данных в C Web Framework. PostgreSQL, MySQL, Redis. Prepared statements, ORM-модели, Query Builder и защита от SQL-инъекций.
---

# База данных

C Web Framework поддерживает PostgreSQL, MySQL и Redis с единым API. Фреймворк обеспечивает connection pooling, prepared statements и защиту от SQL-инъекций.

## Конфигурация

Настройка подключений в `config.json`:

```json
"databases": {
    "postgresql": [{
        "host_id": "p1",
        "ip": "127.0.0.1",
        "port": 5432,
        "dbname": "mydb",
        "user": "dbuser",
        "password": "dbpass",
        "connection_timeout": 3
    }],
    "mysql": [{
        "host_id": "m1",
        "ip": "127.0.0.1",
        "port": 3306,
        "dbname": "mydb",
        "user": "dbuser",
        "password": "dbpass"
    }],
    "redis": [{
        "host_id": "r1",
        "ip": "127.0.0.1",
        "port": 6379,
        "dbindex": 0,
        "user": "",
        "password": ""
    }]
}
```

## Получение подключения

Подключение к базе данных указывается через идентификатор базы данных (первый параметр `dbquery`):

```c
#include "http.h"
#include "db.h"

void handler(httpctx_t* ctx) {
    dbresult_t* result = dbquery("postgresql", "SELECT 1", NULL);

    if (!dbresult_ok(result)) {
        ctx->response->send_data(ctx->response, "Database not available");
        dbresult_free(result);
        return;
    }

    // Работа с результатом...
    dbresult_free(result);
}
```

> Фактическое соединение устанавливается при первом вызове `dbquery`.

## Выполнение запросов

### Простой запрос

```c
void get_users(httpctx_t* ctx) {
    dbresult_t* result = dbquery("postgresql", "SELECT * FROM users LIMIT 10", NULL);

    if (!dbresult_ok(result)) {
        ctx->response->send_data(ctx->response, dbresult_error_message(result));
        dbresult_free(result);
        return;
    }

    // Обработка результатов
    for (int row = 0; row < dbresult_query_rows(result); row++) {
        const db_table_cell_t* id = dbresult_cell(result, row, 0);
        const db_table_cell_t* email = dbresult_cell(result, row, 1);
        printf("User %s: %s\n", id->value, email->value);
    }

    dbresult_free(result);
    ctx->response->send_data(ctx->response, "Done");
}
```

> Данные всегда возвращаются как строки, даже для числовых полей.

### Параметризованные запросы (Prepared Statements)

Безопасный способ передачи параметров с защитой от SQL-инъекций:

```c
#include "query.h"

void get_user_by_id(httpctx_t* ctx) {
    int ok = 0;
    const char* user_id = query_param_char(ctx->request, "id", &ok);

    if (!ok) {
        ctx->response->send_data(ctx->response, "Invalid id parameter");
        return;
    }

    // Использование именованных параметров
    mparams_create_array(params,
        mparam_int(id, atoi(user_id))
    );

    dbresult_t* result = dbquery("postgresql",
        "SELECT * FROM users WHERE id = :id",
        params
    );

    array_free(params);

    if (!dbresult_ok(result)) {
        ctx->response->send_data(ctx->response, "User not found");
        dbresult_free(result);
        return;
    }

    // Обработка результата...
    dbresult_free(result);
}
```

### Несколько параметров

```c
void search_users(httpctx_t* ctx) {
    mparams_create_array(params,
        mparam_int(min_age, 18),
        mparam_int(max_age, 65),
        mparam_text(status, "active")
    );

    dbresult_t* result = dbquery("postgresql",
        "SELECT * FROM users WHERE age BETWEEN :min_age AND :max_age AND status = :status",
        params
    );

    array_free(params);
    // ...
}
```

### Множественные запросы

PostgreSQL и MySQL поддерживают выполнение нескольких запросов:

```c
dbresult_t* result = dbquery("postgresql",
    "SELECT * FROM users LIMIT 5; SELECT * FROM orders LIMIT 5;",
    NULL
);

if (dbresult_ok(result)) {
    // Первый результат (users)
    do {
        for (int row = 0; row < dbresult_query_rows(result); row++) {
            // Обработка строк...
        }
    } while (dbresult_query_next(result)); // Переход к следующему результату
}

dbresult_free(result);
```

## INSERT, UPDATE, DELETE

```c
void create_user(httpctx_t* ctx) {
    char* email = ctx->request->get_payloadf(ctx->request, "email");
    char* name = ctx->request->get_payloadf(ctx->request, "name");

    mparams_create_array(params,
        mparam_text(email, email),
        mparam_text(name, name)
    );

    dbresult_t* result = dbquery("postgresql",
        "INSERT INTO users (email, name) VALUES (:email, :name) RETURNING id",
        params
    );

    array_free(params);

    if (!dbresult_ok(result)) {
        ctx->response->send_data(ctx->response, "Error creating user");
    } else {
        const db_table_cell_t* id = dbresult_cell(result, 0, 0);
        ctx->response->send_data(ctx->response, id->value);
    }

    dbresult_free(result);
    free(email);
    free(name);
}
```

## Работа с моделями (ORM)

Фреймворк поддерживает ORM-подобные модели:

```c
#include "user.h"

void create_user_example(httpctx_t* ctx) {
    // Создание экземпляра модели
    user_t* user = user_instance();

    // Установка значений
    user_set_email(user, "newuser@example.com");
    user_set_name(user, "John Doe");

    // Генерация хеша пароля
    str_t* secret = generate_secret("password123");
    user_set_secret(user, str_get(secret));
    str_free(secret);

    // Сохранение в БД
    if (!user_create(user)) {
        ctx->response->send_data(ctx->response, "Error creating user");
        user_free(user);
        return;
    }

    // Отправка модели как JSON
    ctx->response->send_model(ctx->response, user,
                              display_fields("id", "email", "name"));

    user_free(user);
}
```

### Поиск записей

```c
#include "query.h"

void find_user(httpctx_t* ctx) {
    int ok = 0;
    const char* id = query_param_char(ctx->request, "id", &ok);

    if (!ok) {
        ctx->response->send_default(ctx->response, 400);
        return;
    }

    user_t* user = user_find_by_id(atoi(id));

    if (!user) {
        ctx->response->send_default(ctx->response, 404);
        return;
    }

    ctx->response->send_model(ctx->response, user,
                              display_fields("id", "email", "name", "created_at"));

    user_free(user);
}
```

### Обновление записей

```c
#include "query.h"

void update_user(httpctx_t* ctx) {
    int ok = 0;
    const char* id = query_param_char(ctx->request, "id", &ok);

    if (!ok) {
        ctx->response->send_default(ctx->response, 400);
        return;
    }

    char* name = ctx->request->get_payloadf(ctx->request, "name");

    user_t* user = user_find_by_id(atoi(id));

    if (!user) {
        ctx->response->send_default(ctx->response, 404);
        if (name) free(name);
        return;
    }

    user_set_name(user, name);

    if (!user_update(user)) {
        ctx->response->send_data(ctx->response, "Error updating user");
    } else {
        ctx->response->send_model(ctx->response, user,
                                  display_fields("id", "email", "name"));
    }

    user_free(user);
    free(name);
}
```

## Redis

```c
void cache_example(httpctx_t* ctx) {
    // SET
    dbresult_t* set_result = dbquery("redis", "SET mykey myvalue", NULL);
    dbresult_free(set_result);

    // GET
    dbresult_t* result = dbquery("redis", "GET mykey", NULL);

    if (dbresult_ok(result)) {
        const db_table_cell_t* value = dbresult_cell(result, 0, 0);
        ctx->response->send_data(ctx->response, value->value);
    }

    dbresult_free(result);
}
```

## Подготовленные запросы

Подробное руководство по использованию параметризованных запросов и защите от SQL-инъекций: [Подготовленные запросы](/prepared-statements)

## Миграции

Подробнее о системе миграций: [Миграции баз данных](/migrations)
