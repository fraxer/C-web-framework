---
outline: deep
description: Prepared Statements in C Web Framework. Registration of prepared statements, SQL injection protection, CRUD operation examples.
---

# Prepared Statements

Prepared statements in C Web Framework are a mechanism for pre-registering SQL queries with parameters. They prevent SQL injection and optimize execution through query plan caching.

## Basic Concepts

**Why use prepared statements:**
- ✅ **Security** — protection from SQL injection
- ✅ **Performance** — query plan reuse
- ✅ **Readability** — centralized query definitions
- ✅ **Type safety** — explicit parameter type specification
- ✅ **Templating** — use of substitutions like `@table`, `@list__fields`

## Project Structure

To work with prepared statements, use:

```
project/
├── config.json                         # Database configuration
├── backend/
│   └── app/
│       └── models/
│           └── prepare_statements.c    # Prepared statement registration
├── handlers/
│   └── users.c                         # Handlers
└── migrations/
    └── 001_create_users.sql            # SQL migrations
```

## Step 1: Database Configuration

**File:** `config.json`

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

## Step 2: Create Table

**File:** `migrations/001_create_users.sql`

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

Run migration:
```bash
psql -h 127.0.0.1 -U dbuser -d myapp -f migrations/001_create_users.sql
```

## Step 3: Register Prepared Statements

**File:** `backend/app/models/prepare_statements.c`

### Header and Includes

```c
#include "statement_registry.h"
#include "log.h"
```

### Prepared Statement: Get User by ID

```c
/**
 * Prepared statement: Get user by ID
 * Query: SELECT id, name, email, age FROM user WHERE id = :id
 */
static prepare_stmt_t* pstmt_user_get_by_id(void) {
    prepare_stmt_t* stmt = pstmt_create();
    if (stmt == NULL) return NULL;

    // Step 1: Create parameter array
    array_t* params = array_create();
    if (params == NULL) {
        pstmt_free(stmt);
        return NULL;
    }

    // Step 2: Fill parameters (field definitions and values)
    mparams_fill_array(params,
        mfield_def_int(id),                           // :id — integer
        mfield_def_array(fields, array_create_strings("id", "name", "email", "age"))  // @list__fields
    );

    // Step 3: Set statement name (for identification)
    stmt->name = str_create("user_get_by_id");

    // Step 4: Set SQL with substitutions
    // @list__fields will be replaced with field list
    // :id will be replaced with parameter value
    stmt->query = str_create("SELECT @list__fields FROM \"user\" WHERE id = :id LIMIT 1");

    stmt->params = params;

    return stmt;
}
```

### Prepared Statement: Get User by Email

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

### Prepared Statement: Create User

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

### Prepared Statement: Update User

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

### Prepared Statement: Delete User

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

### Prepared Statement: Search Users

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

### Initialization Function

```c
/**
 * Initialize and register all prepared statements
 * Called during application startup
 */
int prepare_statements_init(void) {
    // Register get user by ID statement
    if (!pstmt_registry_register(pstmt_user_get_by_id)) {
        log_error("prepare_statements_init: failed to register pstmt_user_get_by_id\n");
        return 0;
    }

    // Register get user by email statement
    if (!pstmt_registry_register(pstmt_user_get_by_email)) {
        log_error("prepare_statements_init: failed to register pstmt_user_get_by_email\n");
        return 0;
    }

    // Register create user statement
    if (!pstmt_registry_register(pstmt_user_create)) {
        log_error("prepare_statements_init: failed to register pstmt_user_create\n");
        return 0;
    }

    // Register update user statement
    if (!pstmt_registry_register(pstmt_user_update)) {
        log_error("prepare_statements_init: failed to register pstmt_user_update\n");
        return 0;
    }

    // Register delete user statement
    if (!pstmt_registry_register(pstmt_user_delete)) {
        log_error("prepare_statements_init: failed to register pstmt_user_delete\n");
        return 0;
    }

    // Register search users statement
    if (!pstmt_registry_register(pstmt_user_search)) {
        log_error("prepare_statements_init: failed to register pstmt_user_search\n");
        return 0;
    }

    log_info("prepare_statements_init: all prepared statements registered successfully\n");
    return 1;
}
```

## Step 4: Use Prepared Statements in Handlers

**File:** `handlers/users.c`

### Get User by ID

```c
#include "http.h"
#include "db.h"
#include "query.h"

void get_user(httpctx_t* ctx) {
    // Step 1: Get ID from URL parameters
    int ok = 0;
    int user_id = query_param_int(ctx->request, "id", &ok);

    if (!ok) {
        ctx->response->send_data(ctx->response, "Missing id parameter");
        return;
    }

    // Step 2: Create parameters for prepared statement
    mparams_create_array(params,
        mparam_int(id, user_id)
    );

    // Step 3: Execute prepared statement by name
    dbresult_t* result = dbquery_prepared("postgresql", "user_get_by_id", params);

    // Step 4: Free parameter array
    array_free(params);

    // Step 5: Check result
    if (!dbresult_ok(result)) {
        ctx->response->send_data(ctx->response, "Query error");
        dbresult_free(result);
        return;
    }

    // Step 6: Check if results exist
    if (dbresult_query_rows(result) == 0) {
        ctx->response->send_data(ctx->response, "User not found");
        dbresult_free(result);
        return;
    }

    // Step 7: Build JSON response
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

### Create User

```c
void create_user(httpctx_t* ctx) {
    // Step 1: Get data from request body
    char* name = ctx->request->get_payloadf(ctx->request, "name");
    char* email = ctx->request->get_payloadf(ctx->request, "email");
    char* age_str = ctx->request->get_payloadf(ctx->request, "age");

    // Step 2: Validate input data
    if (!name || !email || !age_str) {
        ctx->response->send_data(ctx->response, "Missing required fields");
        if (name) free(name);
        if (email) free(email);
        if (age_str) free(age_str);
        return;
    }

    // Step 3: Create parameters for prepared statement
    mparams_create_array(params,
        mparam_text(name, name),
        mparam_text(email, email),
        mparam_int(age, atoi(age_str))
    );

    // Step 4: Execute prepared statement
    dbresult_t* result = dbquery_prepared("postgresql", "user_create", params);

    array_free(params);

    // Step 5: Handle errors
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

    // Step 6: Get returned values
    db_table_cell_t* new_id = dbresult_cell(result, 0, 0);
    db_table_cell_t* created_at = dbresult_cell(result, 0, 1);

    // Step 7: Build response
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

### Update User

```c
void update_user(httpctx_t* ctx) {
    // Step 1: Get ID from URL
    int ok = 0;
    int user_id = query_param_int(ctx->request, "id", &ok);

    if (!ok) {
        ctx->response->send_data(ctx->response, "Missing id");
        return;
    }

    // Step 2: Get data to update
    char* name = ctx->request->get_payloadf(ctx->request, "name");
    char* email = ctx->request->get_payloadf(ctx->request, "email");
    char* age_str = ctx->request->get_payloadf(ctx->request, "age");

    if (!name && !email && !age_str) {
        ctx->response->send_data(ctx->response, "No fields to update");
        return;
    }

    // Step 3: Set values (use 0 for missing fields)
    int age = age_str ? atoi(age_str) : 0;
    const char* name_val = name ? name : "";
    const char* email_val = email ? email : "";

    // Step 4: Create parameters
    mparams_create_array(params,
        mparam_int(id, user_id),
        mparam_text(name, name_val),
        mparam_text(email, email_val),
        mparam_int(age, age)
    );

    // Step 5: Execute prepared statement
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

### Delete User

```c
void delete_user(httpctx_t* ctx) {
    // Step 1: Get ID
    int ok = 0;
    int user_id = query_param_int(ctx->request, "id", &ok);

    if (!ok) {
        ctx->response->send_data(ctx->response, "Missing id");
        return;
    }

    // Step 2: Create parameter
    mparams_create_array(params,
        mparam_int(id, user_id)
    );

    // Step 3: Execute prepared statement
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

### Search Users

```c
void search_users(httpctx_t* ctx) {
    // Step 1: Get search parameters
    int ok_name = 0, ok_age = 0;
    const char* search_name = query_param_char(ctx->request, "name", &ok_name);
    int min_age = query_param_int(ctx->request, "min_age", &ok_age);

    // Step 2: Prepare search parameters
    char name_pattern[512];
    snprintf(name_pattern, sizeof(name_pattern), "%%%s%%", ok_name ? search_name : "");

    if (!ok_age) min_age = 0;

    // Step 3: Create parameters
    mparams_create_array(params,
        mparam_text(name_pattern, name_pattern),
        mparam_int(min_age, min_age)
    );

    // Step 4: Execute search prepared statement
    dbresult_t* result = dbquery_prepared("postgresql", "user_search", params);

    array_free(params);

    if (!dbresult_ok(result)) {
        ctx->response->send_data(ctx->response, "Search failed");
        dbresult_free(result);
        return;
    }

    // Step 5: Build JSON array of results
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

### Get User Using Models (model_prepared_one)

For more convenient work with results, you can use models:

```c
// Define user model
typedef struct {
    modelview_t base;
    struct {
        mfield_t id;
        mfield_t name;
        mfield_t email;
        mfield_t age;
    } field;
} user_model_t;

// Function to create model instance
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

// Handler using model
void get_user_model(httpctx_t* ctx) {
    // Step 1: Get ID
    int ok = 0;
    int user_id = query_param_int(ctx->request, "id", &ok);

    if (!ok) {
        ctx->response->send_data(ctx->response, "Missing id");
        return;
    }

    // Step 2: Create parameters
    mparams_create_array(params,
        mparam_int(id, user_id)
    );

    // Step 3: Get single result as model
    user_model_t* user = model_prepared_one("postgresql", user_model_create, "user_get_by_id", params);

    array_free(params);

    if (user == NULL) {
        ctx->response->send_data(ctx->response, "User not found");
        return;
    }

    // Step 4: Send model as JSON
    ctx->response->add_header(ctx->response, "Content-Type", "application/json");
    ctx->response->send_model(ctx->response, user, display_fields("id", "name", "email", "age"));

    model_free(user);
}
```

### Get User List Using Models (model_prepared_list)

To retrieve a list of users:

```c
void search_users_model(httpctx_t* ctx) {
    // Step 1: Get search parameters
    int ok_name = 0, ok_age = 0;
    const char* search_name = query_param_char(ctx->request, "name", &ok_name);
    int min_age = query_param_int(ctx->request, "min_age", &ok_age);

    // Prepare parameters
    char name_pattern[512];
    snprintf(name_pattern, sizeof(name_pattern), "%%%s%%", ok_name ? search_name : "");

    if (!ok_age) min_age = 0;

    // Step 2: Create parameters
    mparams_create_array(params,
        mparam_text(name_pattern, name_pattern),
        mparam_int(min_age, min_age)
    );

    // Step 3: Get list of results as model array
    array_t* users = model_prepared_list("postgresql", user_model_create, "user_search", params);

    array_free(params);

    if (users == NULL || array_length(users) == 0) {
        // If array is NULL, it was freed by the function
        ctx->response->send_data(ctx->response, "[]");
        return;
    }

    // Step 4: Build and send JSON response
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
    array_free(users);  // Frees all models in the array
}
```

## Step 5: Register Handlers in Config

**File:** `config.json`

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

## Parameter Types

When defining parameters, use:

| Function | SQL Type | Usage |
|----------|----------|-------|
| `mfield_def_int(name)` | INTEGER | Integer parameter |
| `mfield_def_text(name)` | VARCHAR/TEXT | Text parameter |
| `mfield_def_array(name, array)` | — | Array for substitution |
| `mparam_int(name, value)` | INTEGER | Parameter value |
| `mparam_text(name, value)` | VARCHAR/TEXT | Parameter value |

## Query Substitutions

| Substitution | Purpose | Example |
|-------------|---------|---------|
| `:name` | Parameter with defined type | `WHERE email = :email` |
| `@list__fields` | Field list from `fields` array | `SELECT @list__fields` |
| `@table` | Table name (not recommended) | `FROM @table` |

## Testing

### Get User

```bash
curl http://localhost:8080/api/users/1
```

### Create User

```bash
curl -X POST http://localhost:8080/api/users \
  -H "Content-Type: application/x-www-form-urlencoded" \
  -d "name=John&email=john@example.com&age=30"
```

### Update User

```bash
curl -X PUT http://localhost:8080/api/users/1 \
  -H "Content-Type: application/x-www-form-urlencoded" \
  -d "name=Jane&email=jane@example.com&age=28"
```

### Delete User

```bash
curl -X DELETE http://localhost:8080/api/users/1
```

### Search Users

```bash
curl "http://localhost:8080/api/users?name=John&min_age=25"
```

## Security

### SQL Injection Protection

Prepared statements automatically protect from SQL injection through parameterization:

```c
// ✅ Safe: parameters are properly escaped
mparams_create_array(params, mparam_text(email, user_input));
dbresult_t* result = dbquery_prepared("postgresql", "user_get_by_email", params);

// ❌ Dangerous: string concatenation (don't use!)
char query[1024];
sprintf(query, "SELECT * FROM user WHERE email = '%s'", user_input);
```

## Best Practices

1. **Register all queries** in `prepare_statements_init()` during app startup
2. **Use descriptive names** for prepared statements: `user_get_by_id`, `user_create`, etc.
3. **Check results** before using data
4. **Free memory** after working with results and parameters
5. **Validate input** before passing to database
6. **Log errors** during registration and execution
7. **Document queries** with comments

## Related Sections

- [Database](/db) — main documentation
- [Working with JSON](/json) — building JSON responses
- [Database Examples](/examples-db) — additional examples
