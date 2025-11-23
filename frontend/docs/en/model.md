---
outline: deep
description: ORM models in C Web Framework. Model definition, CRUD operations, field types, JSON serialization.
---

# ORM Models

The framework provides ORM (Object-Relational Mapping) for working with databases. Models allow you to describe table structures and perform CRUD operations without writing SQL queries.

## Model Definition

### Header File

```c
// app/models/user.h
#ifndef __USER__
#define __USER__

#include "model.h"

typedef struct {
    mfield_t id;
    mfield_t email;
    mfield_t name;
    mfield_t created_at;
} user_fields_t;

typedef struct {
    model_t base;
    user_fields_t field;
    char table[64];
    char* primary_key[1];
} user_t;

void* user_instance(void);
user_t* user_get(array_t* params);
int user_create(user_t* user);
int user_update(user_t* user);
int user_delete(user_t* user);
void user_free(user_t* user);

// Getters
int user_id(user_t* user);
const char* user_email(user_t* user);
const char* user_name(user_t* user);

// Setters
void user_set_id(user_t* user, int id);
void user_set_email(user_t* user, const char* email);
void user_set_name(user_t* user, const char* name);

#endif
```

### Model Implementation

```c
// app/models/user.c
#include "user.h"
#include "db.h"

static const char* __dbid = "postgresql.p1";

static mfield_t* __first_field(void* arg);
static int __fields_count(void* arg);
static const char* __table(void* arg);
static const char** __primary_key(void* arg);
static int __primary_key_count(void* arg);

void* user_instance(void) {
    user_t* user = malloc(sizeof(user_t));
    if (user == NULL) return NULL;

    user_t st = {
        .base = {
            .fields_count = __fields_count,
            .primary_key_count = __primary_key_count,
            .first_field = __first_field,
            .table = __table,
            .primary_key = __primary_key
        },
        .field = {
            mfield_int(id, 0),
            mfield_text(email, NULL),
            mfield_text(name, NULL),
            mfield_timestamp(created_at, (tm_t){0})
        },
        .table = "\"user\"",
        .primary_key = { "id" }
    };

    memcpy(user, &st, sizeof(st));
    return user;
}

user_t* user_get(array_t* params) {
    return model_get(__dbid, user_instance, params);
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

// Getters
int user_id(user_t* user) {
    return model_int(&user->field.id);
}

const char* user_email(user_t* user) {
    return str_get(model_text(&user->field.email));
}

const char* user_name(user_t* user) {
    return str_get(model_text(&user->field.name));
}

// Setters
void user_set_id(user_t* user, int id) {
    model_set_int(&user->field.id, id);
}

void user_set_email(user_t* user, const char* email) {
    model_set_text(&user->field.email, email);
}

void user_set_name(user_t* user, const char* name) {
    model_set_text(&user->field.name, name);
}

// Helper functions
mfield_t* __first_field(void* arg) {
    user_t* user = arg;
    return user ? (mfield_t*)&user->field : NULL;
}

int __fields_count(void* arg) {
    user_t* user = arg;
    return user ? sizeof(user->field) / sizeof(mfield_t) : 0;
}

const char* __table(void* arg) {
    user_t* user = arg;
    return user ? user->table : NULL;
}

const char** __primary_key(void* arg) {
    user_t* user = arg;
    return user ? (const char**)&user->primary_key[0] : NULL;
}

int __primary_key_count(void* arg) {
    user_t* user = arg;
    return user ? sizeof(user->primary_key) / sizeof(user->primary_key[0]) : 0;
}
```

## Field Types

| Macro | PostgreSQL Type | C Type |
|-------|-----------------|--------|
| `mfield_bool` | BOOLEAN | `short` |
| `mfield_smallint` | SMALLINT | `short` |
| `mfield_int` | INTEGER | `int` |
| `mfield_bigint` | BIGINT | `long long` |
| `mfield_float` | REAL | `float` |
| `mfield_double` | DOUBLE PRECISION | `double` |
| `mfield_decimal` | DECIMAL | `long double` |
| `mfield_money` | MONEY | `double` |
| `mfield_date` | DATE | `tm_t` |
| `mfield_time` | TIME | `tm_t` |
| `mfield_timetz` | TIME WITH TIME ZONE | `tm_t` |
| `mfield_timestamp` | TIMESTAMP | `tm_t` |
| `mfield_timestamptz` | TIMESTAMP WITH TIME ZONE | `tm_t` |
| `mfield_json` | JSON/JSONB | `json_doc_t*` |
| `mfield_binary` | BYTEA | `str_t*` |
| `mfield_varchar` | VARCHAR | `str_t*` |
| `mfield_char` | CHAR | `str_t*` |
| `mfield_text` | TEXT | `str_t*` |
| `mfield_enum` | ENUM | `str_t*` |

### Example with Different Types

```c
.field = {
    mfield_int(id, 0),
    mfield_varchar(name, NULL),
    mfield_text(description, NULL),
    mfield_bool(is_active, 1),
    mfield_double(price, 0.0),
    mfield_timestamp(created_at, (tm_t){0}),
    mfield_json(metadata, NULL),
    mfield_enum(status, "pending", "pending", "active", "completed")
}
```

## CRUD Operations

### Creating a Record (Create)

```c
void create_user(httpctx_t* ctx) {
    user_t* user = user_instance();
    if (user == NULL) {
        ctx->response->send_data(ctx->response, "Memory error");
        return;
    }

    user_set_email(user, "user@example.com");
    user_set_name(user, "John Doe");

    if (!user_create(user)) {
        user_free(user);
        ctx->response->send_data(ctx->response, "Failed to create user");
        return;
    }

    char response[64];
    snprintf(response, sizeof(response), "User created with ID: %d", user_id(user));
    ctx->response->send_data(ctx->response, response);

    user_free(user);
}
```

### Retrieving a Record (Read)

```c
void get_user(httpctx_t* ctx) {
    array_t* params = array_create();
    mparams_fill_array(params,
        mparam_int(id, 1)
    );

    user_t* user = user_get(params);
    array_free(params);

    if (user == NULL) {
        ctx->response->send_data(ctx->response, "User not found");
        return;
    }

    char response[256];
    snprintf(response, sizeof(response),
        "ID: %d, Email: %s, Name: %s",
        user_id(user),
        user_email(user),
        user_name(user)
    );

    ctx->response->send_data(ctx->response, response);
    user_free(user);
}
```

### Updating a Record (Update)

```c
void update_user(httpctx_t* ctx) {
    // Get user
    array_t* params = array_create();
    mparams_fill_array(params, mparam_int(id, 1));
    user_t* user = user_get(params);
    array_free(params);

    if (user == NULL) {
        ctx->response->send_data(ctx->response, "User not found");
        return;
    }

    // Update fields
    user_set_name(user, "Jane Doe");

    if (!user_update(user)) {
        user_free(user);
        ctx->response->send_data(ctx->response, "Failed to update user");
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
        ctx->response->send_data(ctx->response, "User not found");
        return;
    }

    if (!user_delete(user)) {
        user_free(user);
        ctx->response->send_data(ctx->response, "Failed to delete user");
        return;
    }

    ctx->response->send_data(ctx->response, "User deleted");
    user_free(user);
}
```

## Custom Queries

### Getting a Single Record

```c
user_t* user_find_by_email(const char* email) {
    array_t* params = array_create();
    mparams_fill_array(params,
        mparam_text(email, email)
    );

    user_t* user = model_one(
        "postgresql.p1",
        user_instance,
        "SELECT * FROM \"user\" WHERE email = $1",
        params
    );

    array_free(params);
    return user;
}
```

### Getting a List of Records

```c
array_t* user_find_active(void) {
    return model_list(
        "postgresql.p1",
        user_instance,
        "SELECT * FROM \"user\" WHERE is_active = true ORDER BY created_at DESC",
        NULL
    );
}

// Usage
void list_users(httpctx_t* ctx) {
    array_t* users = user_find_active();
    if (users == NULL) {
        ctx->response->send_data(ctx->response, "No users found");
        return;
    }

    // Serialize to JSON
    char* json = model_list_stringify(users);
    ctx->response->header_add(ctx->response, "Content-Type", "application/json");
    ctx->response->send_data(ctx->response, json);

    free(json);
    array_free(users);
}
```

## Prepared Statements

```c
// Prepared statement definition
user_t* user_prepared_get(int id) {
    array_t* params = array_create();
    mparams_fill_array(params, mparam_int(id, id));

    user_t* user = model_prepared_one(
        "postgresql.p1",
        user_instance,
        "get_user_by_id",  // Prepared statement name
        params
    );

    array_free(params);
    return user;
}
```

## JSON Serialization

### Single Model

```c
void get_user_json(httpctx_t* ctx) {
    array_t* params = array_create();
    mparams_fill_array(params, mparam_int(id, 1));
    user_t* user = user_get(params);
    array_free(params);

    if (user == NULL) {
        ctx->response->send_data(ctx->response, "{}");
        return;
    }

    // All fields
    char* json = model_stringify(user, NULL);

    // Or only selected fields
    // char* json = model_stringify(user, display_fields("id", "email", "name"));

    ctx->response->header_add(ctx->response, "Content-Type", "application/json");
    ctx->response->send_data(ctx->response, json);

    free(json);
    user_free(user);
}
```

### List of Models

```c
void get_users_json(httpctx_t* ctx) {
    array_t* users = model_list(
        "postgresql.p1",
        user_instance,
        "SELECT * FROM \"user\" LIMIT 10",
        NULL
    );

    char* json = model_list_stringify(users);

    ctx->response->header_add(ctx->response, "Content-Type", "application/json");
    ctx->response->send_data(ctx->response, json);

    free(json);
    array_free(users);
}
```

## Getters and Setters

### Getters by Type

```c
short model_bool(mfield_t* field);
short model_smallint(mfield_t* field);
int model_int(mfield_t* field);
long long model_bigint(mfield_t* field);
float model_float(mfield_t* field);
double model_double(mfield_t* field);
long double model_decimal(mfield_t* field);
tm_t model_timestamp(mfield_t* field);
tm_t model_date(mfield_t* field);
json_doc_t* model_json(mfield_t* field);
str_t* model_varchar(mfield_t* field);
str_t* model_text(mfield_t* field);
```

### Setters by Type

```c
int model_set_bool(mfield_t* field, short value);
int model_set_int(mfield_t* field, int value);
int model_set_bigint(mfield_t* field, long long value);
int model_set_double(mfield_t* field, double value);
int model_set_timestamp(mfield_t* field, tm_t* value);
int model_set_varchar(mfield_t* field, const char* value);
int model_set_text(mfield_t* field, const char* value);
int model_set_json(mfield_t* field, json_doc_t* value);
```

### Setters from String

```c
int model_set_int_from_str(mfield_t* field, const char* value);
int model_set_timestamp_from_str(mfield_t* field, const char* value);
int model_set_json_from_str(mfield_t* field, const char* value);
```
