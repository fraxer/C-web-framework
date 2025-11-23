---
outline: deep
description: Authentication in C Web Framework. Password hashing, sessions, RBAC and route protection.
---

# Authentication

The framework provides tools for implementing user authentication: password hashing (PBKDF2-SHA256), session management, and role-based access control (RBAC).

## Password Hashing

### Generating a Secret

```c
#include "auth.h"

str_t* generate_secret(const char* password);
```

Generates a secure secret from a password using PBKDF2-SHA256 with a random salt.

**Parameters**\
`password` — user's password.

**Return Value**\
String in format `iterations$salt$hash`. Must be freed with `str_free()`.

<br>

### Hashing with Existing Salt

```c
int password_hash(const char* password, unsigned char* salt, int salt_size, unsigned char* hash);
```

Hashes a password with the specified salt.

**Parameters**\
`password` — password.\
`salt` — salt.\
`salt_size` — salt size.\
`hash` — buffer for writing the hash.

**Return Value**\
1 on success, 0 on error.

<br>

### Generating Salt

```c
int generate_salt(unsigned char* salt, int size);
```

Generates a cryptographically secure random salt.

**Parameters**\
`salt` — buffer for writing the salt.\
`size` — salt size.

**Return Value**\
1 on success, 0 on error.

## User Authentication

### By Email and Password

```c
user_t* authenticate(const char* email, const char* password);
```

Authenticates a user by email and password.

**Parameters**\
`email` — user's email.\
`password` — password.

**Return Value**\
Pointer to user model on success, `NULL` on error.

<br>

### By Cookie

```c
user_t* authenticate_by_cookie(httpctx_t* ctx);
```

Authenticates a user by cookie from the request.

**Parameters**\
`ctx` — HTTP request context.

**Return Value**\
Pointer to user model on success, `NULL` on error.

## Usage Examples

### User Registration

```c
#include "http.h"
#include "auth.h"
#include "user.h"

void registration(httpctx_t* ctx) {
    int ok = 0;

    // Get data from form
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

    // Validation
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

    // Generate secret (password hash)
    str_t* secret = generate_secret(password);
    if (secret == NULL) {
        ctx->response->status_code = 500;
        ctx->response->send_data(ctx->response, "Error generating password hash");
        return;
    }

    // Create user
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

### Login

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

    // Authentication
    user_t* user = authenticate(email, password);
    if (user == NULL) {
        ctx->response->status_code = 401;
        ctx->response->send_data(ctx->response, "Invalid credentials");
        return;
    }

    // Create session
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

    // Set cookie
    ctx->response->add_cookie(ctx->response, (cookie_t){
        .name = "session_id",
        .value = session_id,
        .seconds = 86400,  // 24 hours
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

### Logout

```c
void logout(httpctx_t* ctx) {
    const char* session_id = ctx->request->get_cookie(ctx->request, "session_id");

    if (session_id != NULL) {
        session_destroy(session_id);
    }

    // Delete cookie
    ctx->response->add_cookie(ctx->response, (cookie_t){
        .name = "session_id",
        .value = "",
        .seconds = 0,
        .path = "/"
    });

    ctx->response->send_data(ctx->response, "Logged out");
}
```

### Protected Route

```c
void profile(httpctx_t* ctx) {
    // Check authorization via middleware
    middleware(middleware_http_auth(ctx));

    // Get user from context
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

## Role-Based Access Control (RBAC)

### Table Structure

```sql
-- Roles
CREATE TABLE role (
    id SERIAL PRIMARY KEY,
    name VARCHAR(64) UNIQUE NOT NULL
);

-- Permissions
CREATE TABLE permission (
    id SERIAL PRIMARY KEY,
    name VARCHAR(64) UNIQUE NOT NULL
);

-- Role-Permission relationship
CREATE TABLE role_permission (
    role_id INTEGER REFERENCES role(id),
    permission_id INTEGER REFERENCES permission(id),
    PRIMARY KEY (role_id, permission_id)
);

-- User-Role relationship
CREATE TABLE user_role (
    user_id INTEGER REFERENCES "user"(id),
    role_id INTEGER REFERENCES role(id),
    PRIMARY KEY (user_id, role_id)
);
```

### Role Check

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

### Permission Check

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

### Middleware for Role Check

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

// Usage
void admin_panel(httpctx_t* ctx) {
    middleware(
        middleware_http_auth(ctx),
        middleware_require_role(ctx, "admin")
    );

    ctx->response->send_data(ctx->response, "Welcome to admin panel");
}
```

### Middleware for Permission Check

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

// Usage
void delete_post(httpctx_t* ctx) {
    middleware(
        middleware_http_auth(ctx),
        middleware_require_permission(ctx, "posts.delete")
    );

    // Delete post...
}
```

## Validation

### Email Validation

```c
int validate_email(const char* email);
```

Checks if an email address is valid.

### Password Validation

```c
int validate_password(const char* password);
```

Checks if a password meets security requirements.

## Constants

```c
#define SALT_SIZE 16    // Salt size in bytes
#define HASH_SIZE 32    // Hash size in bytes
#define ITERATIONS 10000 // Number of PBKDF2 iterations
```
