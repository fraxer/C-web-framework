---
outline: deep
description: ORM-модели в C Web Framework. Схема, CRUD-операции, типы полей, сериализация в JSON, обработка ошибок.
---

# ORM-модели

Фреймворк предоставляет ORM (Object-Relational Mapping) для работы с базами данных. Модели описывают структуру таблицы на этапе компиляции и выполняют CRUD-операции без ручного написания SQL.

Модель состоит из двух частей:

- **Неизменяемая схема** (`mschema_t`) — таблица, список колонок, первичные ключи. Описывается один раз через `static const` и разделяется всеми экземплярами модели.
- **Экземпляр записи** (`model_t`) — строка данных: массив ячеек полей, размещённый в куче. Создаётся функцией `*_instance()` через `model_init()`.

## Определение модели

### Заголовочный файл

```c
// app/models/user.h
#ifndef __USER__
#define __USER__

#include "model.h"

typedef struct {
    model_t record;   // обязательно первый член структуры
} user_t;

void* user_instance(void);
user_t* user_get(array_t* params);
int user_create(user_t* user);
int user_update(user_t* user);
int user_delete(user_t* user);
void user_free(user_t* user);

// Сеттеры
void user_set_id(user_t* user, int id);
void user_set_email(user_t* user, const char* email);
void user_set_name(user_t* user, const char* name);

// Геттеры
int user_id(user_t* user);
const char* user_email(user_t* user);
const char* user_name(user_t* user);

#endif
```

::: tip
`model_t record` должен быть **первым** членом структуры. Благодаря этому указатель на конкретную модель (`user_t*`) можно передавать там, где функция принимает `void* arg` (как делают все CRUD-функции фреймворка).
:::

### Реализация модели

```c
// app/models/user.c
#include "user.h"
#include "db.h"
#include "str.h"

static const char* __dbid = "postgresql.p1";

// 1. Перечисление индексов колонок (используется в геттерах/сеттерах)
enum user_column {
    USER_COL_ID = 0,
    USER_COL_EMAIL,
    USER_COL_NAME,
    USER_COL_CREATED_AT,
    USER_COLUMNS_COUNT
};

// 2. Описание колонок
static const mcolumn_t __user_columns[USER_COLUMNS_COUNT] = {
    [USER_COL_ID]         = { .name = "id",         .type = MODEL_INT,       .is_primary = 1, .auto_increment = 1 },
    [USER_COL_EMAIL]      = { .name = "email",      .type = MODEL_TEXT },
    [USER_COL_NAME]       = { .name = "name",       .type = MODEL_TEXT },
    [USER_COL_CREATED_AT] = { .name = "created_at", .type = MODEL_TIMESTAMP, .has_default = 1 },
};

// 3. Индексы первичных ключей
static const int __user_primary_keys[] = { USER_COL_ID };

// 4. Единая схема таблицы
static const mschema_t __user_schema = {
    .table = "\"user\"",
    .columns = __user_columns,
    .columns_count = USER_COLUMNS_COUNT,
    .primary_keys = __user_primary_keys,
    .primary_keys_count = 1,
};

void* user_instance(void) {
    user_t* user = calloc(1, sizeof * user);
    if (user == NULL) return NULL;

    if (!model_init(&user->record, &__user_schema)) {
        free(user);
        return NULL;
    }

    return user;
}

user_t* user_get(array_t* params) {
    return model_one(__dbid, user_instance,
        "SELECT id, email, name, created_at FROM \"user\" WHERE id = :id LIMIT 1",
        params);
}

int user_create(user_t* user) {
    return model_create(__dbid, user);
}

int user_update(user_t* user) {
    return model_update(__dbid, user);
}

int user_delete(user_t* user) {
    return model_delete(__dbid, user);
}

void user_free(user_t* user) {
    model_free(user);
}

// Сеттеры
void user_set_id(user_t* user, int id) {
    model_set_int(model_field(&user->record, USER_COL_ID), id);
}

void user_set_email(user_t* user, const char* email) {
    model_set_text(model_field(&user->record, USER_COL_EMAIL), email);
}

void user_set_name(user_t* user, const char* name) {
    model_set_text(model_field(&user->record, USER_COL_NAME), name);
}

// Геттеры
int user_id(user_t* user) {
    return model_int(model_field(&user->record, USER_COL_ID));
}

const char* user_email(user_t* user) {
    return str_get(model_text(model_field(&user->record, USER_COL_EMAIL)));
}

const char* user_name(user_t* user) {
    return str_get(model_text(model_field(&user->record, USER_COL_NAME)));
}
```

Доступ к конкретному полю записи выполняется через `model_field(&user->record, <USER_COL_*>)` — он возвращает `mfield_t*` для запрошенной колонки, который затем передаётся в типизированные геттеры/сеттеры.

## Типы колонок и полей

Тип колонки задаётся значением перечисления `mtype_e` в `mcolumn_t.type`.

| `mtype_e` | Тип C | Тип PostgreSQL | Макрос параметра |
|-----------|-------|----------------|------------------|
| `MODEL_BOOL` | `short` | BOOLEAN | `mparam_bool` |
| `MODEL_SMALLINT` | `short` | SMALLINT | `mparam_smallint` |
| `MODEL_INT` | `int` | INTEGER | `mparam_int` |
| `MODEL_BIGINT` | `long long` | BIGINT | `mparam_bigint` |
| `MODEL_FLOAT` | `float` | REAL | `mparam_float` |
| `MODEL_DOUBLE` | `double` | DOUBLE PRECISION | `mparam_double` |
| `MODEL_DECIMAL` | `long double` | DECIMAL | `mparam_decimal` |
| `MODEL_MONEY` | `double` | MONEY | `mparam_money` |
| `MODEL_DATE` | `tm_t` | DATE | `mparam_date` |
| `MODEL_TIME` | `tm_t` | TIME | `mparam_time` |
| `MODEL_TIMETZ` | `tm_t` | TIME WITH TIME ZONE | `mparam_timetz` |
| `MODEL_TIMESTAMP` | `tm_t` | TIMESTAMP | `mparam_timestamp` |
| `MODEL_TIMESTAMPTZ` | `tm_t` | TIMESTAMP WITH TIME ZONE | `mparam_timestamptz` |
| `MODEL_JSON` | `json_doc_t*` | JSON / JSONB | `mparam_json` |
| `MODEL_BINARY` | `str_t*` | BYTEA | `mparam_binary` |
| `MODEL_VARCHAR` | `str_t*` | VARCHAR | `mparam_varchar` |
| `MODEL_CHAR` | `str_t*` | CHAR | `mparam_char` |
| `MODEL_TEXT` | `str_t*` | TEXT | `mparam_text` |
| `MODEL_ENUM` | `str_t*` | ENUM | `mparam_enum` |
| `MODEL_ARRAY` | `array_t*` | ARRAY | `mparam_array` |

### Атрибуты колонки (`mcolumn_t`)

| Поле | Назначение |
|------|-----------|
| `name` | Имя колонки в таблице |
| `type` | Тип значения (`mtype_e`) |
| `is_primary` | Колонка входит в первичный ключ |
| `has_default` | БД предоставляет значение по умолчанию: колонка пропускается при INSERT, пока не задана явно |
| `nullable` | Числовые/временные колонки стартуют как NULL |
| `auto_increment` | `SERIAL` / `AUTO_INCREMENT` PK: сгенерированный ключ считывается обратно после `model_create` |
| `enum_values`, `enum_count` | Только для `MODEL_ENUM` — список допустимых значений |

### Колонка-перечисление

```c
static const char* const __status_values[] = { "pending", "active", "completed" };

static const mcolumn_t __order_columns[] = {
    [ORDER_COL_ID]     = { .name = "id",     .type = MODEL_INT, .is_primary = 1, .auto_increment = 1 },
    [ORDER_COL_STATUS] = { .name = "status", .type = MODEL_ENUM,
                           .enum_values = __status_values, .enum_count = 3 },
};
```

## Параметры запросов

Параметры собираются в `array_t*` макросом `mparams_fill_array`, который принимает `mparam_*`-выражения. Имя параметра (`#NAME`) становится именованным плейсхолдером `:name` в SQL.

```c
array_t* params = array_create();
mparams_fill_array(params,
    mparam_int(id, 1),
    mparam_text(email, "user@example.com")
);

user_t* user = user_get(params);
array_free(params);   // освобождает и сами параметры
```

::: tip Именованные плейсхолдеры
В тексте SQL используйте `:name` для подготовленных запросов и `model_one`/`model_list`/`dbquery` (параметр подставляется по имени). Позиционный синтаксис `$1` поддерживается драйвером PostgreSQL напрямую.
:::

## CRUD-операции

### Создание записи (Create)

```c
void create_user(httpctx_t* ctx) {
    user_t* user = user_instance();
    if (user == NULL) {
        ctx->response->send_default(ctx->response, 500);
        return;
    }

    user_set_email(user, "user@example.com");
    user_set_name(user, "John Doe");

    if (!user_create(user)) {
        user_free(user);
        ctx->response->send_default(ctx->response, 500);
        return;
    }

    // Для auto_increment PK новый id уже прочитан из БД:
    char response[64];
    snprintf(response, sizeof(response), "User created with ID: %d", user_id(user));
    ctx->response->send_data(ctx->response, response);

    user_free(user);
}
```

### Получение записи (Read)

```c
void get_user(httpctx_t* ctx) {
    int ok = 0;
    const int user_id_value = query_param_int(ctx->request->query_, "id", &ok);
    if (!ok) {
        ctx->response->send_default(ctx->response, 400);
        return;
    }

    array_t* params = array_create();
    mparams_fill_array(params, mparam_int(id, user_id_value));

    user_t* user = user_get(params);
    array_free(params);

    if (user == NULL) {
        if (model_last_status() == MODEL_ERR_NOTFOUND) {
            ctx->response->send_default(ctx->response, 404);
        } else {
            ctx->response->send_default(ctx->response, 500);
        }
        return;
    }

    ctx->response->send_model(ctx->response, user, display_fields("id", "email", "name"));
    user_free(user);
}
```

### Обновление записи (Update)

```c
void update_user(httpctx_t* ctx) {
    array_t* params = array_create();
    mparams_fill_array(params, mparam_int(id, 1));
    user_t* user = user_get(params);
    array_free(params);

    if (user == NULL) {
        ctx->response->send_default(ctx->response, 404);
        return;
    }

    user_set_name(user, "Jane Doe");

    if (!user_update(user)) {
        user_free(user);
        ctx->response->send_default(ctx->response, 500);
        return;
    }

    ctx->response->send_data(ctx->response, "User updated");
    user_free(user);
}
```

### Удаление записи (Delete)

```c
void delete_user(httpctx_t* ctx) {
    array_t* params = array_create();
    mparams_fill_array(params, mparam_int(id, 1));
    user_t* user = user_get(params);
    array_free(params);

    if (user == NULL) {
        ctx->response->send_default(ctx->response, 404);
        return;
    }

    if (!user_delete(user)) {
        user_free(user);
        ctx->response->send_default(ctx->response, 500);
        return;
    }

    ctx->response->send_data(ctx->response, "User deleted");
    user_free(user);
}
```

Удалить несколько строк сразу по параметрам без загрузки записи можно через `model_delete_by_params`:

```c
int deleted = model_delete_by_params(__dbid, user_instance, params);
```

## Обработка ошибок

CRUD-функции сохраняют прежний контракт возврата: `NULL` или `0` означает «не выполнено». Чтобы узнать причину, фреймворк ведёт thread-local статус (в стиле `errno`), который читается сразу после вызова.

```c
typedef enum {
    MODEL_OK = 0,
    MODEL_ERR_NOTFOUND,   // запрос выполнен, строк 0
    MODEL_ERR_DB,         // ошибка драйвера/запроса (см. model_last_error)
    MODEL_ERR_PARAM,      // неверные аргументы (NULL dbid/arg, пустые params)
    MODEL_ERR_ALLOC       // нехватка памяти / ошибка конвертации значения
} model_status_e;

model_status_e model_last_status(void);  // статус последней операции в потоке
const char*    model_last_error(void);   // текст ошибки БД для MODEL_ERR_DB, иначе NULL
```

Статус и текст ошибки действительны только до следующей операции с моделью в том же потоке.

## Пользовательские запросы

### Получение одной записи — `model_one`

```c
user_t* user_find_by_email(const char* email) {
    array_t* params = array_create();
    mparams_fill_array(params, mparam_text(email, email));

    user_t* user = model_one(
        __dbid,
        user_instance,
        "SELECT id, email, name, created_at FROM \"user\" WHERE email = :email LIMIT 1",
        params
    );

    array_free(params);
    return user;
}
```

### Получение списка записей — `model_list`

```c
array_t* user_list_active(void) {
    return model_list(
        __dbid,
        user_instance,
        "SELECT id, email, name, created_at FROM \"user\" "
        "WHERE is_active = true ORDER BY created_at DESC",
        NULL
    );
}

// Использование
void list_users(httpctx_t* ctx) {
    array_t* users = user_list_active();
    if (users == NULL) {
        ctx->response->send_default(ctx->response, 500);
        return;
    }

    ctx->response->send_models(ctx->response, users, display_fields("id", "email", "name"));
    array_free(users);
}
```

## Prepared Statements

Именованные подготовленные запросы выполняются через `model_prepared_one` / `model_prepared_list`. Передаются **имя** подготовленного запроса, **SQL-текст** и параметры.

```c
user_t* user_prepared_get(int id) {
    array_t* params = array_create();
    mparams_fill_array(params, mparam_int(id, id));

    user_t* user = model_prepared_one(
        __dbid,
        user_instance,
        "get_user_by_id",                                                    // имя prepared statement
        "SELECT id, email, name, created_at FROM \"user\" WHERE id = :id LIMIT 1",  // SQL
        params
    );

    array_free(params);
    return user;
}
```

## Сериализация в JSON

### Одна модель

```c
// Все поля
char* json = model_stringify(user, NULL);

// Только выбранные поля
char* json = model_stringify(user, display_fields("id", "email", "name"));

free(json);
```

`display_fields(...)` — это макрос, строящий NULL-терминированный массив `char*[]` имён полей. Передайте `NULL`, чтобы вывести все поля.

Готовый ответ HTTP можно отправить напрямую через метод ответа:

```c
// Все поля
ctx->response->send_model(ctx->response, user, NULL);

// Только выбранные поля
ctx->response->send_model(ctx->response, user, display_fields("id", "email", "name"));
```

### Список моделей

```c
char* json = model_list_stringify(users);
// или напрямую:
ctx->response->send_models(ctx->response, users, NULL);
```

### Сборка вложенного JSON — `model_to_json`

Если нужно встроить модель в более крупный JSON-документ, используйте `model_to_json` — она возвращает `json_token_t*`, который можно добавить в любой объект/массив.

```c
json_token_t* obj = model_to_json(user, display_fields("id", "name"));
json_object_set(parent, "user", obj);
```

## Геттеры и сеттеры

Все функции принимают `mfield_t*`, полученный через `model_field(&record, COL_INDEX)`.

### Геттеры

```c
short        model_bool(mfield_t* field);
short        model_smallint(mfield_t* field);
int          model_int(mfield_t* field);
long long    model_bigint(mfield_t* field);
float        model_float(mfield_t* field);
double       model_double(mfield_t* field);
long double  model_decimal(mfield_t* field);
double       model_money(mfield_t* field);

tm_t         model_timestamp(mfield_t* field);
tm_t         model_timestamptz(mfield_t* field);
tm_t         model_date(mfield_t* field);
tm_t         model_time(mfield_t* field);
tm_t         model_timetz(mfield_t* field);

json_doc_t*  model_json(mfield_t* field);

str_t*       model_binary(mfield_t* field);
str_t*       model_varchar(mfield_t* field);
str_t*       model_char(mfield_t* field);
str_t*       model_text(mfield_t* field);
str_t*       model_enum(mfield_t* field);

array_t*     model_array(mfield_t* field);
```

Строковые геттеры возвращают `str_t*` — используйте `str_get(...)`, чтобы получить `const char*`.

### Сеттеры

```c
int model_set_bool(mfield_t* field, short value);
int model_set_smallint(mfield_t* field, short value);
int model_set_int(mfield_t* field, int value);
int model_set_bigint(mfield_t* field, long long value);
int model_set_float(mfield_t* field, float value);
int model_set_double(mfield_t* field, double value);
int model_set_decimal(mfield_t* field, long double value);
int model_set_money(mfield_t* field, double value);

int model_set_timestamp(mfield_t* field, tm_t* value);
int model_set_timestamp_now(mfield_t* field);
int model_set_timestamptz(mfield_t* field, tm_t* value);
int model_set_timestamptz_now(mfield_t* field);
int model_set_date(mfield_t* field, tm_t* value);
int model_set_time(mfield_t* field, tm_t* value);
int model_set_timetz(mfield_t* field, tm_t* value);

int model_set_json(mfield_t* field, json_doc_t* value);

int model_set_binary(mfield_t* field, const char* value, size_t size);
int model_set_varchar(mfield_t* field, const char* value);
int model_set_char(mfield_t* field, const char* value);
int model_set_text(mfield_t* field, const char* value);
int model_set_enum(mfield_t* field, const char* value);

int model_set_array(mfield_t* field, array_t* value);
```

### Сеттеры из строки

```c
int model_set_bool_from_str(mfield_t* field, const char* value);
int model_set_smallint_from_str(mfield_t* field, const char* value);
int model_set_int_from_str(mfield_t* field, const char* value);
int model_set_bigint_from_str(mfield_t* field, const char* value);
int model_set_float_from_str(mfield_t* field, const char* value);
int model_set_double_from_str(mfield_t* field, const char* value);
int model_set_decimal_from_str(mfield_t* field, const char* value);
int model_set_money_from_str(mfield_t* field, const char* value);

int model_set_timestamp_from_str(mfield_t* field, const char* value);
int model_set_timestamptz_from_str(mfield_t* field, const char* value);
int model_set_date_from_str(mfield_t* field, const char* value);
int model_set_time_from_str(mfield_t* field, const char* value);
int model_set_timetz_from_str(mfield_t* field, const char* value);

int model_set_json_from_str(mfield_t* field, const char* value);

int model_set_binary_from_str(mfield_t* field, const char* value, size_t size);
int model_set_varchar_from_str(mfield_t* field, const char* value, size_t size);
int model_set_char_from_str(mfield_t* field, const char* value, size_t size);
int model_set_text_from_str(mfield_t* field, const char* value, size_t size);
int model_set_enum_from_str(mfield_t* field, const char* value, size_t size);

int model_set_array_from_str(mfield_t* field, const char* value);
```

### Преобразование в строку

```c
str_t* model_field_to_string(mfield_t* field);
str_t* model_json_to_str(mfield_t* field);
str_t* model_array_to_str(mfield_t* field);
// ...и типизированные: model_int_to_str, model_double_to_str,
//     model_timestamp_to_str, model_text_to_str и т. д.
```
