---
outline: deep
description: Подготовленные запросы в C Web Framework. Регистрация подготовленных запросов, защита от SQL-инъекций, примеры CRUD операций.
---

# Подготовленные запросы (Prepared Statements)

Подготовленные запросы в C Web Framework — это механизм предварительной регистрации SQL запросов с параметрами. Они предотвращают SQL-инъекции и оптимизируют выполнение благодаря кешированию плана запроса.

## Основные концепции

**Зачем использовать подготовленные запросы:**
- ✅ **Безопасность** — защита от SQL-инъекций
- ✅ **Производительность** — переиспользование плана запроса
- ✅ **Читаемость** — централизованное определение запросов
- ✅ **Типизация** — явное указание типов параметров
- ✅ **Шаблонизация** — использование подстановок вроде `@table`, `@list__fields`

## Структура проекта

Для работы с подготовленными запросами используются:

```
project/
├── config.json                         # Конфигурация БД
├── backend/
│   └── app/
│       └── models/
│           └── prepare_statements.c    # Регистрация подготовленных запросов
├── handlers/
│   └── users.c                         # Обработчики
└── migrations/
    └── 001_create_users.sql            # SQL миграции
```

## Шаг 1: Конфигурация базы данных

**Файл:** `config.json`

```json
{
  "databases": {
    "postgresql": [
      {
        "host_id": "main",
        "ip": "127.0.0.1",
        "port": 5432,
        "dbname": "myapp",
        "user": "dbuser",
        "password": "dbpass",
        "connection_timeout": 5
      }
    ]
  }
}
```

## Шаг 2: Создание таблицы

**Файл:** `migrations/001_create_users.sql`

```sql
CREATE TABLE IF NOT EXISTS "user" (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    email VARCHAR(255) UNIQUE NOT NULL,
    age INT,
    status VARCHAR(50) DEFAULT 'active',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX idx_email ON "user"(email);
CREATE INDEX idx_name ON "user"(name);
CREATE INDEX idx_status ON "user"(status);
```

Запустите миграцию:
```bash
psql -h 127.0.0.1 -U dbuser -d myapp -f migrations/001_create_users.sql
```

## Шаг 3: Регистрация подготовленных запросов

**Файл:** `backend/app/models/prepare_statements.c`

### Заголовок и включения

```c
#include "statement_registry.h"
#include "log.h"
```

### Подготовленный запрос: получение пользователя

```c
/**
 * Prepared statement: Get user by ID
 * Query: SELECT id, name, email, age FROM user WHERE id = :id
 */
static prepare_stmt_t* pstmt_user_get_by_id(void) {
    prepare_stmt_t* stmt = pstmt_create();
    if (stmt == NULL) return NULL;

    // Шаг 1: Создать массив параметров
    array_t* params = array_create();
    if (params == NULL) {
        pstmt_free(stmt);
        return NULL;
    }

    // Шаг 2: Заполнить параметры (определения полей и значения)
    mparams_fill_array(params,
        mfield_def_int(id),                           // :id — целое число
        mfield_def_array(fields, array_create_strings("id", "name", "email", "age"))  // @list__fields
    );

    // Шаг 3: Установить имя запроса (для идентификации)
    stmt->name = str_create("user_get_by_id");

    // Шаг 4: Установить SQL с подстановками
    // @list__fields будет заменен на список полей
    // :id будет заменен на значение параметра
    stmt->query = str_create("SELECT @list__fields FROM \"user\" WHERE id = :id LIMIT 1");

    stmt->params = params;

    return stmt;
}
```

### Подготовленный запрос: получение пользователя по email

```c
/**
 * Prepared statement: Get user by email
 */
static prepare_stmt_t* pstmt_user_get_by_email(void) {
    prepare_stmt_t* stmt = pstmt_create();
    if (stmt == NULL) return NULL;

    array_t* params = array_create();
    if (params == NULL) {
        pstmt_free(stmt);
        return NULL;
    }

    mparams_fill_array(params,
        mfield_def_text(email),
        mfield_def_array(fields, array_create_strings("id", "name", "email", "age"))
    );

    stmt->name = str_create("user_get_by_email");
    stmt->query = str_create("SELECT @list__fields FROM \"user\" WHERE email = :email LIMIT 1");
    stmt->params = params;

    return stmt;
}
```

### Подготовленный запрос: создание пользователя

```c
/**
 * Prepared statement: Create user
 */
static prepare_stmt_t* pstmt_user_create(void) {
    prepare_stmt_t* stmt = pstmt_create();
    if (stmt == NULL) return NULL;

    array_t* params = array_create();
    if (params == NULL) {
        pstmt_free(stmt);
        return NULL;
    }

    mparams_fill_array(params,
        mfield_def_text(name),
        mfield_def_text(email),
        mfield_def_int(age)
    );

    stmt->name = str_create("user_create");
    stmt->query = str_create(
        "INSERT INTO \"user\" (name, email, age) "
        "VALUES (:name, :email, :age) "
        "RETURNING id, created_at"
    );
    stmt->params = params;

    return stmt;
}
```

### Подготовленный запрос: обновление пользователя

```c
/**
 * Prepared statement: Update user
 */
static prepare_stmt_t* pstmt_user_update(void) {
    prepare_stmt_t* stmt = pstmt_create();
    if (stmt == NULL) return NULL;

    array_t* params = array_create();
    if (params == NULL) {
        pstmt_free(stmt);
        return NULL;
    }

    mparams_fill_array(params,
        mfield_def_int(id),
        mfield_def_text(name),
        mfield_def_text(email),
        mfield_def_int(age)
    );

    stmt->name = str_create("user_update");
    stmt->query = str_create(
        "UPDATE \"user\" "
        "SET name = :name, email = :email, age = :age, updated_at = NOW() "
        "WHERE id = :id "
        "RETURNING id, updated_at"
    );
    stmt->params = params;

    return stmt;
}
```

### Подготовленный запрос: удаление пользователя

```c
/**
 * Prepared statement: Delete user
 */
static prepare_stmt_t* pstmt_user_delete(void) {
    prepare_stmt_t* stmt = pstmt_create();
    if (stmt == NULL) return NULL;

    array_t* params = array_create();
    if (params == NULL) {
        pstmt_free(stmt);
        return NULL;
    }

    mparams_fill_array(params,
        mfield_def_int(id)
    );

    stmt->name = str_create("user_delete");
    stmt->query = str_create("DELETE FROM \"user\" WHERE id = :id");
    stmt->params = params;

    return stmt;
}
```

### Подготовленный запрос: поиск пользователей

```c
/**
 * Prepared statement: Search users
 */
static prepare_stmt_t* pstmt_user_search(void) {
    prepare_stmt_t* stmt = pstmt_create();
    if (stmt == NULL) return NULL;

    array_t* params = array_create();
    if (params == NULL) {
        pstmt_free(stmt);
        return NULL;
    }

    mparams_fill_array(params,
        mfield_def_text(name_pattern),
        mfield_def_int(min_age),
        mfield_def_array(fields, array_create_strings("id", "name", "email", "age"))
    );

    stmt->name = str_create("user_search");
    stmt->query = str_create(
        "SELECT @list__fields FROM \"user\" "
        "WHERE name ILIKE :name_pattern AND age >= :min_age "
        "ORDER BY name LIMIT 50"
    );
    stmt->params = params;

    return stmt;
}
```

### Функция инициализации

```c
/**
 * Initialize and register all prepared statements
 * Called during application startup
 */
int prepare_statements_init(void) {
    // Регистрируем подготовленный запрос для получения пользователя по ID
    if (!pstmt_registry_register(pstmt_user_get_by_id)) {
        log_error("prepare_statements_init: failed to register pstmt_user_get_by_id\n");
        return 0;
    }

    // Регистрируем подготовленный запрос для получения пользователя по email
    if (!pstmt_registry_register(pstmt_user_get_by_email)) {
        log_error("prepare_statements_init: failed to register pstmt_user_get_by_email\n");
        return 0;
    }

    // Регистрируем подготовленный запрос для создания пользователя
    if (!pstmt_registry_register(pstmt_user_create)) {
        log_error("prepare_statements_init: failed to register pstmt_user_create\n");
        return 0;
    }

    // Регистрируем подготовленный запрос для обновления пользователя
    if (!pstmt_registry_register(pstmt_user_update)) {
        log_error("prepare_statements_init: failed to register pstmt_user_update\n");
        return 0;
    }

    // Регистрируем подготовленный запрос для удаления пользователя
    if (!pstmt_registry_register(pstmt_user_delete)) {
        log_error("prepare_statements_init: failed to register pstmt_user_delete\n");
        return 0;
    }

    // Регистрируем подготовленный запрос для поиска пользователей
    if (!pstmt_registry_register(pstmt_user_search)) {
        log_error("prepare_statements_init: failed to register pstmt_user_search\n");
        return 0;
    }

    log_info("prepare_statements_init: all prepared statements registered successfully\n");
    return 1;
}
```

## Шаг 4: Использование подготовленных запросов в обработчиках

**Файл:** `handlers/users.c`

### Получение пользователя по ID

```c
#include "http.h"
#include "db.h"
#include "query.h"

void get_user(httpctx_t* ctx) {
    // Шаг 1: Получить ID из URL параметров
    int ok = 0;
    int user_id = query_param_int(ctx->request, "id", &ok);

    if (!ok) {
        ctx->response->send_data(ctx->response, "Missing id parameter");
        return;
    }

    // Шаг 2: Создать параметры для подготовленного запроса
    mparams_create_array(params,
        mparam_int(id, user_id)
    );

    // Шаг 3: Выполнить подготовленный запрос по имени
    dbresult_t* result = dbquery_prepared("postgresql", "user_get_by_id", params);

    // Шаг 4: Освободить массив параметров
    array_free(params);

    // Шаг 5: Проверить результат
    if (!dbresult_ok(result)) {
        ctx->response->send_data(ctx->response, "Query error");
        dbresult_free(result);
        return;
    }

    // Шаг 6: Проверить наличие результатов
    if (dbresult_query_rows(result) == 0) {
        ctx->response->send_data(ctx->response, "User not found");
        dbresult_free(result);
        return;
    }

    // Шаг 7: Построить JSON ответ
    json_doc_t* doc = json_init();
    json_token_t* root = json_create_object(doc);

    db_table_cell_t* id_cell = dbresult_cell(result, 0, 0);
    db_table_cell_t* name_cell = dbresult_cell(result, 0, 1);
    db_table_cell_t* email_cell = dbresult_cell(result, 0, 2);
    db_table_cell_t* age_cell = dbresult_cell(result, 0, 3);

    json_object_set(root, "id", json_create_number(doc, atoi(id_cell->value)));
    json_object_set(root, "name", json_create_string(doc, name_cell->value));
    json_object_set(root, "email", json_create_string(doc, email_cell->value));
    json_object_set(root, "age", json_create_number(doc, atoi(age_cell->value)));

    ctx->response->add_header(ctx->response, "Content-Type", "application/json");
    ctx->response->send_data(ctx->response, json_stringify(doc));

    json_free(doc);
    dbresult_free(result);
}
```

### Создание пользователя

```c
void create_user(httpctx_t* ctx) {
    // Шаг 1: Получить данные из тела запроса
    char* name = ctx->request->get_payloadf(ctx->request, "name");
    char* email = ctx->request->get_payloadf(ctx->request, "email");
    char* age_str = ctx->request->get_payloadf(ctx->request, "age");

    // Шаг 2: Валидировать входные данные
    if (!name || !email || !age_str) {
        ctx->response->send_data(ctx->response, "Missing required fields");
        if (name) free(name);
        if (email) free(email);
        if (age_str) free(age_str);
        return;
    }

    // Шаг 3: Создать параметры для подготовленного запроса
    mparams_create_array(params,
        mparam_text(name, name),
        mparam_text(email, email),
        mparam_int(age, atoi(age_str))
    );

    // Шаг 4: Выполнить подготовленный запрос
    dbresult_t* result = dbquery_prepared("postgresql", "user_create", params);

    array_free(params);

    // Шаг 5: Обработать ошибки
    if (!dbresult_ok(result)) {
        const char* error = dbresult_error_message(result);
        if (strstr(error, "unique")) {
            ctx->response->send_data(ctx->response, "Email already exists");
        } else {
            ctx->response->send_data(ctx->response, "Insert failed");
        }
        dbresult_free(result);
        free(name);
        free(email);
        free(age_str);
        return;
    }

    // Шаг 6: Получить возвращённые значения
    db_table_cell_t* new_id = dbresult_cell(result, 0, 0);
    db_table_cell_t* created_at = dbresult_cell(result, 0, 1);

    // Шаг 7: Построить ответ
    json_doc_t* doc = json_init();
    json_token_t* root = json_create_object(doc);

    json_object_set(root, "success", json_create_bool(doc, 1));
    json_object_set(root, "id", json_create_number(doc, atoi(new_id->value)));
    json_object_set(root, "created_at", json_create_string(doc, created_at->value));

    ctx->response->add_header(ctx->response, "Content-Type", "application/json");
    ctx->response->send_data(ctx->response, json_stringify(doc));

    json_free(doc);
    dbresult_free(result);
    free(name);
    free(email);
    free(age_str);
}
```

### Обновление пользователя

```c
void update_user(httpctx_t* ctx) {
    // Шаг 1: Получить ID из URL
    int ok = 0;
    int user_id = query_param_int(ctx->request, "id", &ok);

    if (!ok) {
        ctx->response->send_data(ctx->response, "Missing id");
        return;
    }

    // Шаг 2: Получить данные для обновления
    char* name = ctx->request->get_payloadf(ctx->request, "name");
    char* email = ctx->request->get_payloadf(ctx->request, "email");
    char* age_str = ctx->request->get_payloadf(ctx->request, "age");

    if (!name && !email && !age_str) {
        ctx->response->send_data(ctx->response, "No fields to update");
        return;
    }

    // Шаг 3: Установить значения (используем 0 для пропущенных полей)
    int age = age_str ? atoi(age_str) : 0;
    const char* name_val = name ? name : "";
    const char* email_val = email ? email : "";

    // Шаг 4: Создать параметры
    mparams_create_array(params,
        mparam_int(id, user_id),
        mparam_text(name, name_val),
        mparam_text(email, email_val),
        mparam_int(age, age)
    );

    // Шаг 5: Выполнить подготовленный запрос
    dbresult_t* result = dbquery_prepared("postgresql", "user_update", params);

    array_free(params);

    if (!dbresult_ok(result)) {
        ctx->response->send_data(ctx->response, "Update failed");
        dbresult_free(result);
        if (name) free(name);
        if (email) free(email);
        if (age_str) free(age_str);
        return;
    }

    if (dbresult_query_rows(result) == 0) {
        ctx->response->send_data(ctx->response, "User not found");
        dbresult_free(result);
        if (name) free(name);
        if (email) free(email);
        if (age_str) free(age_str);
        return;
    }

    ctx->response->send_data(ctx->response, "User updated");
    dbresult_free(result);
    if (name) free(name);
    if (email) free(email);
    if (age_str) free(age_str);
}
```

### Удаление пользователя

```c
void delete_user(httpctx_t* ctx) {
    // Шаг 1: Получить ID
    int ok = 0;
    int user_id = query_param_int(ctx->request, "id", &ok);

    if (!ok) {
        ctx->response->send_data(ctx->response, "Missing id");
        return;
    }

    // Шаг 2: Создать параметр
    mparams_create_array(params,
        mparam_int(id, user_id)
    );

    // Шаг 3: Выполнить подготовленный запрос
    dbresult_t* result = dbquery_prepared("postgresql", "user_delete", params);

    array_free(params);

    if (!dbresult_ok(result)) {
        ctx->response->send_data(ctx->response, "Delete failed");
        dbresult_free(result);
        return;
    }

    ctx->response->send_data(ctx->response, "User deleted");
    dbresult_free(result);
}
```

### Поиск пользователей

```c
void search_users(httpctx_t* ctx) {
    // Шаг 1: Получить параметры поиска
    int ok_name = 0, ok_age = 0;
    const char* search_name = query_param_char(ctx->request, "name", &ok_name);
    int min_age = query_param_int(ctx->request, "min_age", &ok_age);

    // Шаг 2: Подготовить параметры поиска
    char name_pattern[512];
    snprintf(name_pattern, sizeof(name_pattern), "%%%s%%", ok_name ? search_name : "");

    if (!ok_age) min_age = 0;

    // Шаг 3: Создать параметры
    mparams_create_array(params,
        mparam_text(name_pattern, name_pattern),
        mparam_int(min_age, min_age)
    );

    // Шаг 4: Выполнить подготовленный запрос поиска
    dbresult_t* result = dbquery_prepared("postgresql", "user_search", params);

    array_free(params);

    if (!dbresult_ok(result)) {
        ctx->response->send_data(ctx->response, "Search failed");
        dbresult_free(result);
        return;
    }

    // Шаг 5: Построить JSON массив результатов
    json_doc_t* doc = json_init();
    json_token_t* root = json_create_array(doc);

    for (int row = 0; row < dbresult_query_rows(result); row++) {
        json_token_t* user_obj = json_create_object(doc);

        db_table_cell_t* id = dbresult_cell(result, row, 0);
        db_table_cell_t* name = dbresult_cell(result, row, 1);
        db_table_cell_t* email = dbresult_cell(result, row, 2);
        db_table_cell_t* age = dbresult_cell(result, row, 3);

        json_object_set(user_obj, "id", json_create_number(doc, atoi(id->value)));
        json_object_set(user_obj, "name", json_create_string(doc, name->value));
        json_object_set(user_obj, "email", json_create_string(doc, email->value));
        json_object_set(user_obj, "age", json_create_number(doc, atoi(age->value)));

        json_array_push(root, user_obj);
    }

    ctx->response->add_header(ctx->response, "Content-Type", "application/json");
    ctx->response->send_data(ctx->response, json_stringify(doc));

    json_free(doc);
    dbresult_free(result);
}
```

### Получение пользователя с использованием моделей (model_prepared_one)

Для более удобной работы с результатами можно использовать модели:

```c
// Определяем модель пользователя
typedef struct {
    modelview_t base;
    struct {
        mfield_t id;
        mfield_t name;
        mfield_t email;
        mfield_t age;
    } field;
} user_model_t;

// Функция для создания экземпляра модели
void* user_model_create(void) {
    user_model_t* user = malloc(sizeof *user);
    if (user == NULL) return NULL;

    user_model_t st = {
        .base = {
            .fields_count = __user_fields_count,
            .first_field = __user_first_field,
        },
        .field = {
            mfield_int(id, 0),
            mfield_text(name, NULL),
            mfield_text(email, NULL),
            mfield_int(age, 0),
        }
    };

    memcpy(user, &st, sizeof st);
    return user;
}

// Обработчик с использованием модели
void get_user_model(httpctx_t* ctx) {
    // Шаг 1: Получить ID
    int ok = 0;
    int user_id = query_param_int(ctx->request, "id", &ok);

    if (!ok) {
        ctx->response->send_data(ctx->response, "Missing id");
        return;
    }

    // Шаг 2: Создать параметры
    mparams_create_array(params,
        mparam_int(id, user_id)
    );

    // Шаг 3: Получить один результат как модель
    user_model_t* user = model_prepared_one("postgresql", user_model_create, "user_get_by_id", params);

    array_free(params);

    if (user == NULL) {
        ctx->response->send_data(ctx->response, "User not found");
        return;
    }

    // Шаг 4: Отправить модель как JSON
    ctx->response->add_header(ctx->response, "Content-Type", "application/json");
    ctx->response->send_model(ctx->response, user, display_fields("id", "name", "email", "age"));

    model_free(user);
}
```

### Получение списка пользователей с использованием моделей (model_prepared_list)

Для получения списка пользователей:

```c
void search_users_model(httpctx_t* ctx) {
    // Шаг 1: Получить параметры поиска
    int ok_name = 0, ok_age = 0;
    const char* search_name = query_param_char(ctx->request, "name", &ok_name);
    int min_age = query_param_int(ctx->request, "min_age", &ok_age);

    // Подготовить параметры
    char name_pattern[512];
    snprintf(name_pattern, sizeof(name_pattern), "%%%s%%", ok_name ? search_name : "");

    if (!ok_age) min_age = 0;

    // Шаг 2: Создать параметры
    mparams_create_array(params,
        mparam_text(name_pattern, name_pattern),
        mparam_int(min_age, min_age)
    );

    // Шаг 3: Получить список результатов как массив моделей
    array_t* users = model_prepared_list("postgresql", user_model_create, "user_search", params);

    array_free(params);

    if (users == NULL || array_length(users) == 0) {
        // Если массив NULL, он был освобожден функцией
        ctx->response->send_data(ctx->response, "[]");
        return;
    }

    // Шаг 4: Построить и отправить JSON ответ
    json_doc_t* doc = json_init();
    json_token_t* root = json_create_array(doc);

    for (int i = 0; i < array_length(users); i++) {
        user_model_t* user = (user_model_t*)array_get(users, i);
        if (user != NULL) {
            json_token_t* user_json = model_to_json(user, display_fields("id", "name", "email", "age"));
            json_array_push(root, user_json);
        }
    }

    ctx->response->add_header(ctx->response, "Content-Type", "application/json");
    ctx->response->send_data(ctx->response, json_stringify(doc));

    json_free(doc);
    array_free(users);  // Освобождает все модели в массиве
}
```

## Шаг 5: Регистрация обработчиков в конфиге

**Файл:** `config.json`

```json
{
  "routes": {
    "/api/users/{id|\\d+}": {
      "GET": {
        "file": "handlers/libusers.so",
        "function": "get_user"
      },
      "PUT": {
        "file": "handlers/libusers.so",
        "function": "update_user"
      },
      "DELETE": {
        "file": "handlers/libusers.so",
        "function": "delete_user"
      }
    },
    "/api/users": {
      "POST": {
        "file": "handlers/libusers.so",
        "function": "create_user"
      },
      "GET": {
        "file": "handlers/libusers.so",
        "function": "search_users"
      }
    },
    "/api/users/model/{id|\\d+}": {
      "GET": {
        "file": "handlers/libusers.so",
        "function": "get_user_model"
      }
    },
    "/api/users/model/search": {
      "GET": {
        "file": "handlers/libusers.so",
        "function": "search_users_model"
      }
    }
  }
}
```

## Типы параметров

При определении параметров используйте:

| Функция | Тип в SQL | Использование |
|---------|-----------|-----------------|
| `mfield_def_int(name)` | INTEGER | Параметр целого числа |
| `mfield_def_text(name)` | VARCHAR/TEXT | Параметр текста |
| `mfield_def_array(name, array)` | — | Массив для подстановки |
| `mparam_int(name, value)` | INTEGER | Значение параметра |
| `mparam_text(name, value)` | VARCHAR/TEXT | Значение параметра |

## Подстановки в запросах

| Подстановка | Назначение | Пример |
|------------|-----------|---------|
| `:name` | Параметр с определённым типом | `WHERE email = :email` |
| `@list__fields` | Список полей из массива `fields` | `SELECT @list__fields` |
| `@table` | Имя таблицы (не рекомендуется) | `FROM @table` |

## Тестирование

### Получение пользователя

```bash
curl http://localhost:8080/api/users/1
```

### Создание пользователя

```bash
curl -X POST http://localhost:8080/api/users \
  -H "Content-Type: application/x-www-form-urlencoded" \
  -d "name=John&email=john@example.com&age=30"
```

### Обновление пользователя

```bash
curl -X PUT http://localhost:8080/api/users/1 \
  -H "Content-Type: application/x-www-form-urlencoded" \
  -d "name=Jane&email=jane@example.com&age=28"
```

### Удаление пользователя

```bash
curl -X DELETE http://localhost:8080/api/users/1
```

### Поиск пользователей

```bash
curl "http://localhost:8080/api/users?name=John&min_age=25"
```

## Безопасность

### Защита от SQL-инъекций

Подготовленные запросы автоматически защищают от SQL-инъекций благодаря параметризации:

```c
// ✅ Безопасно: параметры проходят экранирование
mparams_create_array(params, mparam_text(email, user_input));
dbresult_t* result = dbquery_prepared("postgresql", "user_get_by_email", params);

// ❌ Опасно: конкатенация строк (не используйте!)
char query[1024];
sprintf(query, "SELECT * FROM user WHERE email = '%s'", user_input);
```

## Лучшие практики

1. **Регистрируйте все запросы** в `prepare_statements_init()` при инициализации приложения
2. **Используйте говорящие имена** для подготовленных запросов: `user_get_by_id`, `user_create`, и т.д.
3. **Проверяйте результаты** перед использованием данных
4. **Освобождайте память** после работы с результатами и параметрами
5. **Валидируйте входные данные** перед передачей в БД
6. **Логируйте ошибки** при регистрации и выполнении запросов
7. **Документируйте запросы** с помощью комментариев

## Связанные разделы

- [База данных](/db) — основная документация
- [Работа с JSON](/json) — построение JSON ответов
- [Примеры работы с БД](/examples-db) — дополнительные примеры
