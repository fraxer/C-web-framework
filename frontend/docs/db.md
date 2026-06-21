---
outline: deep
description: Работа с базами данных в C Web Framework. PostgreSQL, MySQL, Redis, SQLite. Параметризованные запросы, ORM-модели, транзакции и защита от SQL-инъекций.
---

# База данных

C Web Framework поддерживает **PostgreSQL**, **MySQL**, **Redis** и **SQLite** через единый API. Фреймворк обеспечивает пул соединений (по одному на каждый воркер), параметризованные запросы и защиту от SQL-инъекций.

## Конфигурация

Подключения настраиваются в `config.json` в секции `databases`. Каждый драйвер содержит массив хостов — у каждого свой `host_id`:

```json
"databases": {
    "postgresql": [{
        "host_id": "p1",
        "ip": "127.0.0.1",
        "port": 5432,
        "dbname": "mydb",
        "user": "dbuser",
        "password": "dbpass",
        "connection_timeout": 3,
        "schema": "public"
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
    }],
    "sqlite": [{
        "host_id": "s1",
        "path": "/var/data/app.db",
        "journal_mode": "WAL",
        "busy_timeout": 5000
    }]
}
```

::: tip Имена драйверов
В коде доступны константы: `POSTGRESQL`, `MYSQL`, `REDIS`, `SQLITE` (равны строкам `"postgresql"`, `"mysql"`, `"redis"`, `"sqlite"`).
:::

## Идентификатор базы данных (dbid)

База данных, к которой выполняется запрос, задаётся идентификатором **dbid** — первым параметром любой DB-функции. Формат:

```
<драйвер>.<host_id>
```

Например, `postgresql.p1` обращается к PostgreSQL-хосту с `host_id = "p1"`, а `sqlite.s1` — к SQLite-хосту `s1`.

::: tip Короткая форма
Если указать только драйвер (`"postgresql"`), будет выбран первый сконфигурированный хост этого драйвера. Рекомендуется всегда указывать `host_id` явно — это однозначно и безопасно при нескольких хостах.
:::

## Соединение

Фактическое соединение устанавливается **лениво** — при первом запросе к данному dbid и поддерживается в пуле соединений воркера. Вам не нужно открывать или закрывать соединение вручную — достаточно вызывать `dbquery`:

```c
#include "http.h"
#include "db.h"

void handler(httpctx_t* ctx) {
    dbresult_t* result = dbquery("postgresql.p1", "SELECT 1", NULL);

    if (!dbresult_ok(result)) {
        ctx->response->send_data(ctx->response, "Database not available");
        dbresult_free(result);
        return;
    }

    // Работа с результатом...
    dbresult_free(result);
}
```

::: warning Управление памятью
Каждый `dbresult_t*` обязан освобождаться через `dbresult_free(result)`. Завершайте handler через `goto failed;` или `return` так, чтобы освобождение выполнялось на любом пути выполнения.
:::

## Выполнение запросов

### Простой запрос

```c
void get_users(httpctx_t* ctx) {
    dbresult_t* result = dbquery("postgresql.p1",
        "SELECT id, name, email FROM \"user\" LIMIT 10", NULL);

    if (!dbresult_ok(result)) {
        ctx->response->send_data(ctx->response, dbresult_error(result));
        goto failed;
    }

    for (int row = 0; row < dbresult_query_rows(result); row++) {
        const db_table_cell_t* id    = dbresult_cell(result, row, 0);
        const db_table_cell_t* name  = dbresult_cell(result, row, 1);
        printf("id=%s name=%.*s\n", id->value, (int)name->length, name->value);
    }

    ctx->response->send_data(ctx->response, "Done");

    failed:
    dbresult_free(result);
}
```

::: tip Значения всегда строки
Данные возвращаются как строки (`char* value`) вместе с длиной `length`. Числа, даты и логические поля также приходят в текстовом виде — преобразуйте их при необходимости.
:::

### Чтение результата

| Функция | Назначение |
| --- | --- |
| `dbresult_ok(result)` | `1`, если запрос выполнен успешно |
| `dbresult_error(result)` | Текст ошибки (если есть) |
| `dbresult_query_rows(result)` | Количество строк в текущем наборе |
| `dbresult_query_cols(result)` | Количество колонок |
| `dbresult_col_name(result, col)` | Имя колонки по индексу |
| `dbresult_cell(result, row, col)` | Ячейка по индексам строки и колонки |
| `dbresult_field(result, "name")` | Ячейка по имени колонки (из текущей строки) |
| `dbresult_query_next(result)` | Перейти к следующему набору (multi-query) |
| `dbresult_insert_id(result)` | Последний автоинкрементный `id` (insert) |
| `dbresult_free(result)` | Освободить результат |

Структура ячейки `db_table_cell_t` содержит два поля: `size_t length` и `char* value`.

### Параметризованные запросы

Параметры защищают от SQL-инъекций: значение подставляется не в текст SQL, а передаётся отдельно и биндится драйвером. Параметры собираются в массив `array_t*` с помощью макросов:

```c
#include "http.h"
#include "db.h"
#include "query.h"

void get_user(httpctx_t* ctx) {
    int ok = 0;
    const int user_id = query_param_int(ctx->request->query_, "id", &ok);

    if (!ok) {
        ctx->response->send_default(ctx->response, 400);
        return;
    }

    array_t* params = array_create();
    mparams_fill_array(params,
        mparam_int(id, user_id)
    );

    dbresult_t* result = dbquery("postgresql.p1",
        "SELECT id, name, email FROM \"user\" WHERE id = :id",
        params
    );

    array_free(params);

    if (!dbresult_ok(result)) {
        ctx->response->send_default(ctx->response, 500);
        goto failed;
    }

    if (dbresult_query_rows(result) == 0) {
        ctx->response->send_default(ctx->response, 404);
        goto failed;
    }

    db_table_cell_t* name = dbresult_field(result, "name");
    ctx->response->send_data(ctx->response, name ? name->value : "");

    failed:
    dbresult_free(result);
}
```

#### Синтаксис параметров

| Запись | Назначение |
| --- | --- |
| `:name` | **Значение** — биндится как параметр запроса (защита от инъекций) |
| `@name` | **Идентификатор** — имя таблицы/колонки, экранируется и подставляется в текст SQL |

Используйте `:name` для данных и `@name` — только для динамических имён схем/таблиц/колонок (например, `SELECT * FROM @table`).

#### Типы параметров

Макросы `mparam_*` создают типизированные параметры. Имя параметра (`#NAME`) становится строкой и должно совпадать с плейсхолдером в SQL:

| Макрос | Тип |
| --- | --- |
| `mparam_bool(name, v)` | `bool` |
| `mparam_smallint(name, v)` | `smallint` |
| `mparam_int(name, v)` | `int` |
| `mparam_bigint(name, v)` | `bigint` |
| `mparam_float(name, v)` | `float` |
| `mparam_double(name, v)` | `double` |
| `mparam_decimal(name, v)` | `decimal` |
| `mparam_money(name, v)` | `money` (`double`) |
| `mparam_date(name, v)` | `date` |
| `mparam_time(name, v)` | `time` |
| `mparam_timestamp(name, v)` | `timestamp` |
| `mparam_timestamptz(name, v)` | `timestamp with time zone` |
| `mparam_json(name, v)` | `json` |
| `mparam_binary(name, v)` | `binary` / `blob` |
| `mparam_varchar(name, v)` | `varchar` |
| `mparam_char(name, v)` | `char` |
| `mparam_text(name, v)` | `text` |
| `mparam_enum(name, v, ...)` | `enum` |
| `mparam_array(name, v)` | `array` |

Несколько параметров передаются одной инструкцией:

```c
array_t* params = array_create();
mparams_fill_array(params,
    mparam_int(min_age, 18),
    mparam_int(max_age, 65),
    mparam_text(status, "active")
);

dbresult_t* result = dbquery("postgresql.p1",
    "SELECT * FROM \"user\" WHERE age BETWEEN :min_age AND :max_age AND status = :status",
    params
);

array_free(params);
```

### Множественные запросы

PostgreSQL и MySQL поддерживают выполнение нескольких запросов в одном вызове. Каждый результат обходится через `dbresult_query_next`:

```c
dbresult_t* result = dbquery("postgresql.p1",
    "SELECT * FROM \"user\" LIMIT 5; SELECT * FROM \"order\" LIMIT 5;", NULL);

if (dbresult_ok(result)) {
    do {
        for (int row = 0; row < dbresult_query_rows(result); row++) {
            for (int col = 0; col < dbresult_query_cols(result); col++) {
                const db_table_cell_t* field = dbresult_cell(result, row, col);
                printf("%s | ", field->value);
            }
            printf("\n");
        }
    } while (dbresult_query_next(result));
}

dbresult_free(result);
```

## INSERT, UPDATE, DELETE

```c
void create_user(httpctx_t* ctx) {
    char* name  = ctx->request->get_payloadf(ctx->request, "name");
    char* email = ctx->request->get_payloadf(ctx->request, "email");

    if (!name || !email) {
        ctx->response->send_default(ctx->response, 400);
        free(name); free(email);
        return;
    }

    array_t* params = array_create();
    mparams_fill_array(params,
        mparam_text(name, name),
        mparam_text(email, email)
    );

    dbresult_t* result = dbquery("postgresql.p1",
        "INSERT INTO \"user\" (name, email) VALUES (:name, :email) RETURNING id",
        params
    );

    array_free(params);

    if (!dbresult_ok(result)) {
        ctx->response->send_data(ctx->response, dbresult_error(result));
    } else {
        db_table_cell_t* id = dbresult_field(result, "id");
        ctx->response->send_data(ctx->response, id ? id->value : "0");
    }

    dbresult_free(result);
    free(name);
    free(email);
}
```

::: tip RETURNING и автоинкремент
Для PostgreSQL/SQLite используйте `RETURNING id`, либо получите последний вставленный `id` через `dbresult_insert_id(result)`.
:::

## Вспомогательные функции запросов

Поверх `dbquery` есть набор функций для типовых операций с таблицами:

| Функция | Назначение |
| --- | --- |
| `dbinsert(dbid, table, params)` | Вставка строки |
| `dbupdate(dbid, table, set, where)` | Обновление по условию |
| `dbdelete(dbid, table, where)` | Удаление по условию |
| `dbselect(dbid, table, columns, where)` | Выборка колонок по условию |
| `dbexec(dbid, sql, params)` | Выполнить без возврата строк (возвращает `int`) |
| `dbprepared(dbid, name, sql, params)` | Именованный prepared statement |
| `dbtable_exist(dbid, table)` | Проверка существования таблицы |

```c
// Вставка
array_t* row = array_create();
mparams_fill_array(row,
    mparam_text(name, "John Doe"),
    mparam_text(email, "john@example.com")
);
dbresult_t* res = dbinsert("postgresql.p1", "\"user\"", row);
array_free(row);
dbresult_free(res);

// Выборка: columns и where — это array_t*
array_t* columns = array_create();
array_push_back(columns, array_create_string("*"));
array_t* where = array_create();
mparams_fill_array(where, mparam_int(id, 42));

dbresult_t* sel = dbselect("postgresql.p1", "\"user\"", columns, where);
// ... обработка sel ...
array_free(columns);
array_free(where);
dbresult_free(sel);
```

### Prepared statements

`dbprepared` регистрирует prepared statement по имени при первом вызове и переиспользует его при последующих (в рамках соединения):

```c
array_t* params = array_create();
mparams_fill_array(params,
    mparam_int(id, user_id),
    mparam_text(email, "admin@example.com")
);

dbresult_t* result = dbprepared("postgresql.p1", "user_get",
    "SELECT id, name, email FROM \"user\" WHERE id = :id AND email = :email LIMIT 1",
    params);

array_free(params);
dbresult_free(result);
```

## Транзакции

Транзакции управляются функциями `dbbegin`, `dbcommit`, `dbrollback`. Уровень изоляции задаётся перечислением `transaction_level_e`: `READ_UNCOMMITTED`, `READ_COMMITTED`, `REPEATABLE_READ`, `SERIALIZABLE`.

```c
dbresult_t* b = dbbegin("postgresql.p1", READ_COMMITTED);
dbresult_free(b);

dbresult_t* r1 = dbquery("postgresql.p1",
    "UPDATE account SET balance = balance - 100 WHERE id = :id", debit);
dbresult_free(r1);

dbresult_t* r2 = dbquery("postgresql.p1",
    "UPDATE account SET balance = balance + 100 WHERE id = :id", credit);
dbresult_free(r2);

// Фиксируем или откатываем по результату
dbresult_free(dbcommit("postgresql.p1"));
// При ошибке: dbresult_free(dbrollback("postgresql.p1"));
```

## Redis

Redis использует тот же API `dbquery` — команды передаются как SQL-подобный текст:

```c
void cache_example(httpctx_t* ctx) {
    // SET
    dbresult_t* set_result = dbquery("redis.r1", "SET mykey myvalue", NULL);
    dbresult_free(set_result);

    // GET
    dbresult_t* result = dbquery("redis.r1", "GET mykey", NULL);

    if (dbresult_ok(result)) {
        db_table_cell_t* value = dbresult_field(result, NULL);
        if (value) ctx->response->send_data(ctx->response, value->value);
    }

    dbresult_free(result);
}
```

## Модели (ORM)

Фреймворк предоставляет ORM-слой на основе схем (`mschema_t`) и типизированных моделей. На уровне приложения для каждой модели генерируются обёртки — например, для `user`: `user_instance`, `user_get`, `user_create`, `user_update`, `user_delete`, `user_free`.

### Создание

```c
#include "user.h"
#include "auth.h"

void create_user_example(httpctx_t* ctx) {
    user_t* user = user_instance();

    user_set_name(user, "John Doe");
    user_set_email(user, "john@example.com");

    // Хеш пароля (PBKDF2-HMAC-SHA256)
    str_t* secret = generate_secret("password123");
    user_set_secret(user, str_get(secret));
    str_free(secret);

    if (!user_create(user)) {
        ctx->response->send_data(ctx->response, "Error creating user");
        user_free(user);
        return;
    }

    ctx->response->send_model(ctx->response, user,
                              display_fields("id", "email", "name"));

    user_free(user);
}
```

### Получение записи

Для поиска соберите массив параметров — имена параметров становятся условиями `WHERE ... = :name`:

```c
void find_user(httpctx_t* ctx) {
    int ok = 0;
    const int id = query_param_int(ctx->request->query_, "id", &ok);

    if (!ok) {
        ctx->response->send_default(ctx->response, 400);
        return;
    }

    array_t* params = array_create();
    mparams_fill_array(params, mparam_int(id, id));

    user_t* user = user_get(params);
    array_free(params);

    if (!user) {
        ctx->response->send_default(ctx->response, 404);
        return;
    }

    ctx->response->send_model(ctx->response, user,
                              display_fields("id", "email", "name", "created_at"));

    user_free(user);
}
```

### Обновление и удаление

```c
void update_user(httpctx_t* ctx) {
    int ok = 0;
    const int id = query_param_int(ctx->request->query_, "id", &ok);
    char* name = ctx->request->get_payloadf(ctx->request, "name");

    if (!ok || !name) {
        ctx->response->send_default(ctx->response, 400);
        free(name);
        return;
    }

    array_t* params = array_create();
    mparams_fill_array(params, mparam_int(id, id));
    user_t* user = user_get(params);
    array_free(params);

    if (!user) {
        ctx->response->send_default(ctx->response, 404);
        free(name);
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

::: tip Низкоуровневые функции моделей
Универсальные функции доступны напрямую: `model_create`, `model_update`, `model_delete`, `model_one`, `model_list`, `model_prepared_one`, `model_prepared_list`. Диагностика ошибок — через `model_last_status()` (`MODEL_OK`, `MODEL_ERR_NOTFOUND`, `MODEL_ERR_DB`, `MODEL_ERR_PARAM`, `MODEL_ERR_ALLOC`) и `model_last_error()`.
:::

## См. также

- [Подготовленные запросы](/prepared-statements) — детально о параметризации и защите от SQL-инъекций
- [Модели (ORM)](/model) — определение моделей, схемы и поля
- [Миграции баз данных](/migrations) — система миграций
