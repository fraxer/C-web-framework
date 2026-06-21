---
outline: deep
description: ORM models in C Web Framework. Schema definition, CRUD operations, field types, JSON serialization, error handling.
---

# ORM Models

The framework provides ORM (Object-Relational Mapping) for working with databases. Models describe a table structure at compile time and perform CRUD operations without writing SQL by hand.

A model has two parts:

- **Immutable schema** (`mschema_t`) — table, column list, primary keys. Declared once as a `static const` and shared by every instance of the model.
- **Record instance** (`model_t`) — a row of data: a heap-allocated array of field cells. Created by the `*_instance()` function via `model_init()`.

## Model Definition

### Header File

```c
// app/models/user.h
#ifndef __USER__
#define __USER__

#include "model.h"

typedef struct {
    model_t record;   // must be the first member
} user_t;

void* user_instance(void);
user_t* user_get(array_t* params);
int user_create(user_t* user);
int user_update(user_t* user);
int user_delete(user_t* user);
void user_free(user_t* user);

// Setters
void user_set_id(user_t* user, int id);
void user_set_email(user_t* user, const char* email);
void user_set_name(user_t* user, const char* name);

// Getters
int user_id(user_t* user);
const char* user_email(user_t* user);
const char* user_name(user_t* user);

#endif
```

::: tip
`model_t record` must be the **first** member of the struct. Because of this, a pointer to the concrete model (`user_t*`) can be passed wherever a function takes `void* arg` (as all framework CRUD functions do).
:::

### Model Implementation

```c
// app/models/user.c
#include "user.h"
#include "db.h"
#include "str.h"

static const char* __dbid = "postgresql.p1";

// 1. Column-index enum (used by getters/setters)
enum user_column {
    USER_COL_ID = 0,
    USER_COL_EMAIL,
    USER_COL_NAME,
    USER_COL_CREATED_AT,
    USER_COLUMNS_COUNT
};

// 2. Column descriptors
static const mcolumn_t __user_columns[USER_COLUMNS_COUNT] = {
    [USER_COL_ID]         = { .name = "id",         .type = MODEL_INT,       .is_primary = 1, .auto_increment = 1 },
    [USER_COL_EMAIL]      = { .name = "email",      .type = MODEL_TEXT },
    [USER_COL_NAME]       = { .name = "name",       .type = MODEL_TEXT },
    [USER_COL_CREATED_AT] = { .name = "created_at", .type = MODEL_TIMESTAMP, .has_default = 1 },
};

// 3. Primary-key indexes
static const int __user_primary_keys[] = { USER_COL_ID };

// 4. The single table schema
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

// Setters
void user_set_id(user_t* user, int id) {
    model_set_int(model_field(&user->record, USER_COL_ID), id);
}

void user_set_email(user_t* user, const char* email) {
    model_set_text(model_field(&user->record, USER_COL_EMAIL), email);
}

void user_set_name(user_t* user, const char* name) {
    model_set_text(model_field(&user->record, USER_COL_NAME), name);
}

// Getters
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

Access to a specific field of the record is done via `model_field(&user->record, <USER_COL_*>)`, which returns the `mfield_t*` for the requested column and is then passed to the typed getters/setters.

## Column and Field Types

A column's type is set with a value of the `mtype_e` enum in `mcolumn_t.type`.

| `mtype_e` | C type | PostgreSQL type | Param macro |
|-----------|--------|-----------------|-------------|
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

### Column attributes (`mcolumn_t`)

| Field | Meaning |
|-------|---------|
| `name` | Column name in the table |
| `type` | Value type (`mtype_e`) |
| `is_primary` | Column is part of the primary key |
| `has_default` | The DB provides a default value: the column is skipped on INSERT until set explicitly |
| `nullable` | Numeric/temporal columns start as NULL |
| `auto_increment` | `SERIAL` / `AUTO_INCREMENT` PK: the generated key is read back after `model_create` |
| `enum_values`, `enum_count` | `MODEL_ENUM` only — the list of allowed values |

### Enum column

```c
static const char* const __status_values[] = { "pending", "active", "completed" };

static const mcolumn_t __order_columns[] = {
    [ORDER_COL_ID]     = { .name = "id",     .type = MODEL_INT, .is_primary = 1, .auto_increment = 1 },
    [ORDER_COL_STATUS] = { .name = "status", .type = MODEL_ENUM,
                           .enum_values = __status_values, .enum_count = 3 },
};
```

## Query Parameters

Parameters are collected into an `array_t*` with the `mparams_fill_array` macro, which takes `mparam_*` expressions. The parameter name (`#NAME`) becomes the named placeholder `:name` in SQL.

```c
array_t* params = array_create();
mparams_fill_array(params,
    mparam_int(id, 1),
    mparam_text(email, "user@example.com")
);

user_t* user = user_get(params);
array_free(params);   // frees the parameters too
```

::: tip Named placeholders
Use `:name` in the SQL text for prepared queries and `model_one` / `model_list` / `dbquery` (the parameter is bound by name). The positional `$1` syntax is supported by the PostgreSQL driver directly.
:::

## CRUD Operations

### Creating a Record (Create)

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

    // For an auto_increment PK the new id is already read back from the DB:
    char response[64];
    snprintf(response, sizeof(response), "User created with ID: %d", user_id(user));
    ctx->response->send_data(ctx->response, response);

    user_free(user);
}
```

### Retrieving a Record (Read)

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

### Updating a Record (Update)

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

### Deleting a Record (Delete)

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

To delete several rows by params without loading the record first, use `model_delete_by_params`:

```c
int deleted = model_delete_by_params(__dbid, user_instance, params);
```

## Error Handling

CRUD functions keep their existing return contract: `NULL` or `0` means "did not succeed". To tell *why*, the framework keeps a thread-local status (errno-style) that you read right after the call.

```c
typedef enum {
    MODEL_OK = 0,
    MODEL_ERR_NOTFOUND,   /* query ran, returned 0 rows */
    MODEL_ERR_DB,         /* driver/query error (see model_last_error) */
    MODEL_ERR_PARAM,      /* invalid arguments (NULL dbid/arg, empty params) */
    MODEL_ERR_ALLOC       /* out of memory / value conversion failure */
} model_status_e;

model_status_e model_last_status(void);  /* status of the last op in this thread */
const char*    model_last_error(void);   /* DB error text for MODEL_ERR_DB, else NULL */
```

The status and error text are valid only until the next model operation in the same thread.

## Custom Queries

### Single Record — `model_one`

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

### List of Records — `model_list`

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

// Usage
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

Named prepared statements run through `model_prepared_one` / `model_prepared_list`. You pass the prepared statement **name**, the **SQL text**, and the params.

```c
user_t* user_prepared_get(int id) {
    array_t* params = array_create();
    mparams_fill_array(params, mparam_int(id, id));

    user_t* user = model_prepared_one(
        __dbid,
        user_instance,
        "get_user_by_id",                                                    // statement name
        "SELECT id, email, name, created_at FROM \"user\" WHERE id = :id LIMIT 1",  // SQL
        params
    );

    array_free(params);
    return user;
}
```

## JSON Serialization

### Single Model

```c
// All fields
char* json = model_stringify(user, NULL);

// Only selected fields
char* json = model_stringify(user, display_fields("id", "email", "name"));

free(json);
```

`display_fields(...)` is a macro that builds a NULL-terminated `char*[]` of field names. Pass `NULL` to output all fields.

To send a ready-made HTTP response, use the response method directly:

```c
// All fields
ctx->response->send_model(ctx->response, user, NULL);

// Only selected fields
ctx->response->send_model(ctx->response, user, display_fields("id", "email", "name"));
```

### List of Models

```c
char* json = model_list_stringify(users);
// or directly:
ctx->response->send_models(ctx->response, users, NULL);
```

### Building Nested JSON — `model_to_json`

When you need to embed a model into a larger JSON document, use `model_to_json` — it returns a `json_token_t*` you can attach to any object/array.

```c
json_token_t* obj = model_to_json(user, display_fields("id", "name"));
json_object_set(parent, "user", obj);
```

## Getters and Setters

All functions take a `mfield_t*` obtained via `model_field(&record, COL_INDEX)`.

### Getters

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

String getters return `str_t*` — use `str_get(...)` to get a `const char*`.

### Setters

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

### Setters from String

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

### Conversion to String

```c
str_t* model_field_to_string(mfield_t* field);
str_t* model_json_to_str(mfield_t* field);
str_t* model_array_to_str(mfield_t* field);
// ...plus typed variants: model_int_to_str, model_double_to_str,
//     model_timestamp_to_str, model_text_to_str, and so on.
```
