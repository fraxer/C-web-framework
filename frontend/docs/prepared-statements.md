---
outline: deep
description: Подготовленные и параметризованные запросы в C Web Framework. Параметры :name и @name, списки :list__ и @list__, именованные подготовленные запросы dbprepared и model_prepared_one, защита от SQL-инъекций.
---

# Подготовленные запросы (Prepared Statements)

Подготовленные (параметризованные) запросы отделяют значения от текста SQL: данные передаются отдельно и подставляются драйвером через плейсхолдеры. Это даёт защиту от SQL-инъекций и возможность переиспользовать разобранный план запроса.

C Web Framework предоставляет три способа выполнения:

- `dbquery(dbid, sql, params)` — разовый параметризованный запрос.
- `dbprepared(dbid, name, sql, params)` — **именованный** подготовленный запрос: при первом вызове с данным `name` запрос подготавливается и кешируется в соединении, при последующих — переиспользуется.
- `model_prepared_one` / `model_prepared_list` — то же, что и `dbprepared`, но результат возвращается сразу в виде типизированной ORM-модели (см. [Модели](/model) и раздел ниже).

## Зачем использовать

- ✅ **Безопасность** — значения биндятся как параметры, а не вставляются в текст SQL.
- ✅ **Производительность** — `dbprepared` кеширует план запроса по имени в рамках соединения.
- ✅ **Типизация** — параметры типизируются через макросы `mparam_*`.
- ✅ **Динамические идентификаторы** — имена таблиц/колонок можно подставлять безопасно через `@name`.

## Синтаксис параметров

Параметры в SQL обозначаются специальными префиксами:

| Запись | Назначение |
| --- | --- |
| `:name` | **Значение** — биндится как плейсхолдер (`$1`, `$2`, …). Защищено от инъекций. |
| `@name` | **Идентификатор** — имя таблицы/колонки/схемы, экранируется и подставляется в текст SQL. |
| `:list__name` | **Список значений** — раскрывается в список плейсхолдеров через запятую (`$1, $2, $3`). |
| `@list__name` | **Список идентификаторов** — раскрывается в список экранированных имён через запятую. |

::: tip Два вида параметров
`:name` — для **данных** (значения). `@name` — для **имён** объектов БД (таблиц, колонок, схем). Никогда не подставляйте значения через `@` — они не биндятся и не защищены от инъекций.
:::

## Создание параметров

Параметры собираются в массив `array_t*` макросами `mparam_*`. Имя параметра (`#NAME`) становится строкой и должно совпадать с плейсхолдером в SQL:

```c
array_t* params = array_create();
mparams_fill_array(params,
    mparam_int(id, user_id),
    mparam_text(status, "active")
);

// ... использовать params ...

array_free(params);
```

Полный список типов `mparam_*` (`mparam_int`, `mparam_text`, `mparam_bool`, `mparam_double`, `mparam_array`, …) приведён в разделе [База данных](/db#типы-параметров).

::: tip mfield_def_* — определение формы
Макросы `mfield_def_int(name)`, `mfield_def_text(name)` и т. д. создают типизированный параметр со значением по умолчанию — это удобно для описания формы запроса. Для выполнения с реальными данными используйте `mparam_*(name, value)`.
:::

## Разовый запрос — dbquery

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
        "SELECT id, name, email FROM \"user\" WHERE id = :id LIMIT 1",
        params
    );

    array_free(params);

    if (!dbresult_ok(result) || dbresult_query_rows(result) == 0) {
        ctx->response->send_default(ctx->response, 404);
        goto failed;
    }

    db_table_cell_t* name = dbresult_field(result, "name");
    ctx->response->send_data(ctx->response, name ? name->value : "");

    failed:
    dbresult_free(result);
}
```

## Именованный подготовленный запрос — dbprepared

```c
dbresult_t* dbprepared(const char* dbid, const char* name, const char* sql, array_t* params);
```

`dbprepared` подготавливает запрос по имени при первом вызове и переиспользует его при последующих — план запроса кешируется в соединении. Аргумент `sql` нужен только для первой подготовки (при последующих вызовах с тем же `name` он игнорируется).

```c
#include "http.h"
#include "db.h"
#include "query.h"

void get_user_prepared(httpctx_t* ctx) {
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

    // Первый вызов подготовит запрос "user_get_by_id" и закеширует его.
    // Последующие вызовы с тем же именем переиспользуют подготовленный запрос.
    dbresult_t* result = dbprepared("postgresql.p1", "user_get_by_id",
        "SELECT id, name, email FROM \"user\" WHERE id = :id LIMIT 1",
        params
    );

    array_free(params);

    if (!dbresult_ok(result) || dbresult_query_rows(result) == 0) {
        ctx->response->send_default(ctx->response, 404);
        goto failed;
    }

    db_table_cell_t* name = dbresult_field(result, "name");
    ctx->response->send_data(ctx->response, name ? name->value : "");

    failed:
    dbresult_free(result);
}
```

::: tip Когда применять dbprepared
Используйте `dbprepared` для запросов, которые выполняются многократно в одном соединении — это экономит разбор плана. Для одиночных запросов достаточно `dbquery`. Имя должно быть уникальным в пределах соединения.
:::

## Примеры CRUD

### Создание (INSERT … RETURNING)

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

    dbresult_t* result = dbprepared("postgresql.p1", "user_create",
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

::: tip Получить id новой записи
Кроме `RETURNING id`, автоинкрементный ключ доступен через `dbresult_insert_id(result)` — это пригодится, когда SQL не возвращает строк (например, MySQL/SQLite без `RETURNING`).
:::

### Обновление (UPDATE)

```c
void update_user(httpctx_t* ctx) {
    int ok = 0;
    const int user_id = query_param_int(ctx->request->query_, "id", &ok);
    char* name = ctx->request->get_payloadf(ctx->request, "name");
    if (!ok || !name) {
        ctx->response->send_default(ctx->response, 400);
        free(name);
        return;
    }

    array_t* params = array_create();
    mparams_fill_array(params,
        mparam_text(name, name),
        mparam_int(id, user_id)
    );

    dbresult_t* result = dbprepared("postgresql.p1", "user_update",
        "UPDATE \"user\" SET name = :name WHERE id = :id",
        params
    );

    array_free(params);

    if (!dbresult_ok(result)) {
        ctx->response->send_data(ctx->response, "Update failed");
    } else {
        ctx->response->send_data(ctx->response, "User updated");
    }

    dbresult_free(result);
    free(name);
}
```

### Удаление (DELETE)

```c
void delete_user(httpctx_t* ctx) {
    int ok = 0;
    const int user_id = query_param_int(ctx->request->query_, "id", &ok);
    if (!ok) {
        ctx->response->send_default(ctx->response, 400);
        return;
    }

    array_t* params = array_create();
    mparams_fill_array(params, mparam_int(id, user_id));

    dbresult_t* result = dbprepared("postgresql.p1", "user_delete",
        "DELETE FROM \"user\" WHERE id = :id",
        params
    );

    array_free(params);
    dbresult_free(result);

    ctx->response->send_data(ctx->response, "Deleted");
}
```

### Поиск (LIKE)

```c
void search_users(httpctx_t* ctx) {
    int ok = 0;
    const char* q = query_param_char(ctx->request->query_, "q", &ok);
    if (!ok) {
        ctx->response->send_default(ctx->response, 400);
        return;
    }

    char pattern[512];
    snprintf(pattern, sizeof(pattern), "%%%s%%", q);

    array_t* params = array_create();
    mparams_fill_array(params, mparam_text(pattern, pattern));

    dbresult_t* result = dbquery("postgresql.p1",
        "SELECT id, name, email FROM \"user\" WHERE name ILIKE :pattern LIMIT 10",
        params
    );

    array_free(params);

    if (!dbresult_ok(result)) {
        ctx->response->send_data(ctx->response, "Search failed");
        goto failed;
    }

    for (int row = 0; row < dbresult_query_rows(result); row++) {
        const db_table_cell_t* name = dbresult_cell(result, row, 1);
        printf("%s\n", name->value);
    }

    ctx->response->send_data(ctx->response, "Done");

    failed:
    dbresult_free(result);
}
```

## Динамические идентификаторы и списки

Иногда имя таблицы/колонки или набор значений заранее неизвестны. Для этого используются `@name`, `@list__name` и `:list__name`.

### Динамические имена (`@name`, `@list__name`)

`@name` подставляет экранированный идентификатор, а `@list__name` — список идентификаторов через запятую. Источник — параметр-массив (`mparam_array`):

```c
array_t* fields = array_create_strings("id", "name", "email");
array_t* params = array_create();
mparams_fill_array(params,
    mparam_array(fields, fields),   // @list__fields -> "id", "name", "email"
    mparam_text(table, "user")      // @table        -> "user"
);

dbresult_t* result = dbquery("postgresql.p1",
    "SELECT @list__fields FROM @table WHERE id = :id",
    params
);

array_free(params);
// mparam_array не копирует массив: params владеет fields.
// Освобождайте только params — НЕ вызывайте array_free(fields) дополнительно.
dbresult_free(result);
```

### Список значений (`:list__name`)

`:list__name` раскрывается в список плейсхолдеров — удобно для `IN (...)`:

```c
array_t* id_arr = array_create_from_ints((int[]){ 1, 5, 9 }, 3);

array_t* params = array_create();
mparams_fill_array(params, mparam_array(id, id_arr));

dbresult_t* result = dbquery("postgresql.p1",
    "SELECT * FROM \"user\" WHERE id IN (:list__id)",
    params
);
// Раскрывается в: SELECT * FROM "user" WHERE id IN ($1, $2, $3)

array_free(params);
// params владеет id_arr — отдельно array_free(id_arr) вызывать не нужно.
dbresult_free(result);
```

::: warning Владение массивом
`mparam_array(name, arr)` **не копирует** массив — владение переходит к `params`. Освобождайте только `params` через `array_free(params)`. Не освобождайте внутренний массив отдельно — это приведёт к двойному освобождению.
:::

## Защита от SQL-инъекций

Параметры `:name` и `:list__name` биндятся драйвером как значения, а не вставляются в текст SQL, поэтому вредоносный ввод остаётся данными и не может изменить структуру запроса:

```c
// Опасно — значение вставляется в текст SQL (инъекция!):
// snprintf(sql, "SELECT * FROM \"user\" WHERE email = '%s'", user_input);

// Безопасно — значение биндится как параметр:
array_t* params = array_create();
mparams_fill_array(params, mparam_text(email, user_input));
dbresult_t* result = dbquery("postgresql.p1",
    "SELECT * FROM \"user\" WHERE email = :email", params);
array_free(params);
```

::: tip Правило
Все значения — через `:name`. Все динамические имена объектов — через `@name` (они экранируются как идентификаторы). Никогда не собирайте SQL из пользовательского ввода строковыми функциями.
:::

## Подготовленные запросы и модели (ORM)

`dbquery` / `dbprepared` возвращают «сырые» ячейки (`db_table_cell_t`). Если в приложении уже определена ORM-модель (см. [Модели](/model)), удобнее получить сразу типизированный объект — для этого есть `model_prepared_one` / `model_prepared_list`:

```c
#include "http.h"
#include "db.h"
#include "query.h"
#include "model.h"
#include "user.h"   // приложение: user_instance, user_t

void get_user_model(httpctx_t* ctx) {
    int ok = 0;
    const int user_id = query_param_int(ctx->request->query_, "id", &ok);
    if (!ok) {
        ctx->response->send_default(ctx->response, 400);
        return;
    }

    array_t* params = array_create();
    mparams_fill_array(params, mparam_int(id, user_id));

    // Первый вызов подготовит "user_get_by_id" и закеширует его в соединении.
    // Результат — типизированная модель user_t* (или NULL при ошибке / not found).
    user_t* user = model_prepared_one("postgresql.p1", user_instance,
        "user_get_by_id",
        "SELECT id, name, email FROM \"user\" WHERE id = :id LIMIT 1",
        params
    );

    array_free(params);

    if (user == NULL) {
        // Причина доступна через model_last_status():
        //   MODEL_ERR_NOTFOUND -> 404, MODEL_ERR_DB -> 500.
        ctx->response->send_default(ctx->response,
            model_last_status() == MODEL_ERR_NOTFOUND ? 404 : 500);
        return;
    }

    ctx->response->send_model(ctx->response, user,
        display_fields("id", "email", "name"));

    model_free(user);
}
```

Для нескольких строк используется `model_prepared_list` — она возвращает `array_t*` моделей и отправляется через `send_models`:

```c
array_t* users = model_prepared_list("postgresql.p1", user_instance,
    "users_active",
    "SELECT id, name, email FROM \"user\" WHERE status = :status",
    params
);

if (users != NULL) {
    ctx->response->send_models(ctx->response, users,
        display_fields("id", "email", "name"));
    array_free(users);
}
```

::: tip Когда NULL — это не ошибка
`model_prepared_one` возвращает `NULL` и при «не найдено», и при ошибке БД. Различить их позволяет `model_last_status()` (`MODEL_OK`, `MODEL_ERR_NOTFOUND`, `MODEL_ERR_DB`, `MODEL_ERR_PARAM`, `MODEL_ERR_ALLOC`), а текст ошибки — `model_last_error()`.
:::

## См. также

- [База данных](/db) — конфигурация, `dbquery`, типы параметров, транзакции
- [Модели (ORM)](/model) — `model_prepared_one` / `model_prepared_list` для типизированных моделей
- [Миграции баз данных](/migrations) — система миграций
