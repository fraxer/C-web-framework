---
outline: deep
description: Authentication in C Web Framework. Password hashing, sessions, RBAC and route protection.
---

# Authentication

The framework provides tools for implementing user authentication: password hashing (PBKDF2-HMAC-SHA256), session management, credential validation, and role-based access control (RBAC).

## Password Hashing

Hashing uses PBKDF2-HMAC-SHA256. The salt and hash are stored together in a single `user` field formatted as `iterations$salt$hash` (the salt and hash are hex-encoded). Default parameters are defined in `auth.h`:

```c
#define SALT_SIZE 16      // salt size in bytes
#define HASH_SIZE 32      // hash size in bytes
#define ITERATIONS 140000 // number of PBKDF2 iterations
```

### Generating a Secret

```c
#include "auth.h"

str_t* generate_secret(const char* password);
```

Generates a secure secret from a password: creates a random salt, hashes the password, and assembles the `iterations$salt$hash` string.

**Parameters**\
`password` — user's password.

**Return Value**\
String in the format `iterations$salt$hash`. Must be freed with `str_free()`. `NULL` on failure.

<br>

### Assembling a Secret From Existing Values

```c
str_t* create_secret(int iterations, const char* hash_hex, const char* salt_hex);
```

Assembles a secret from an iteration count, a hash, and a salt (both hex strings), separated by `$`. Used internally by `generate_secret()`, but also available for direct use — for example, when changing the iteration count.

**Parameters**\
`iterations` — number of PBKDF2 iterations.\
`hash_hex` — password hash as hex.\
`salt_hex` — salt as hex.

**Return Value**\
The secret string. Must be freed with `str_free()`. `NULL` on failure.

<br>

### Hashing with Existing Salt

```c
int password_hash(const char* password, unsigned char* salt, int salt_size, unsigned char* hash);
```

Hashes a password with the given salt and `ITERATIONS` iterations.

**Parameters**\
`password` — password.\
`salt` — salt.\
`salt_size` — salt size.\
`hash` — buffer of `HASH_SIZE` bytes for writing the hash.

**Return Value**\
1 on success, 0 on error.

<br>

### Generating Salt

```c
int generate_salt(unsigned char* salt, int size);
```

Generates a cryptographically secure random salt (OpenSSL `RAND_bytes`).

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

Validates the email and password, loads the user from the database, extracts the salt from the secret, hashes the password, and compares it with the stored hash.

**Parameters**\
`email` — user's email.\
`password` — password.

**Return Value**\
Pointer to the user model on success, `NULL` on error (invalid input, user not found, hash mismatch). The user must be freed with `user_free()`.

<br>

### By `token` Cookie

```c
user_t* authenticate_by_cookie(httpctx_t* ctx);
```

Authenticates a user by the `token` cookie from the request — performs a user lookup by token. Suited to long-lived access-token schemes (e.g. "remember me") as an alternative to sessions.

**Parameters**\
`ctx` — HTTP request context.

**Return Value**\
Pointer to the user model on success, `NULL` on error.

::: tip Two authentication mechanisms
The framework supports two independent mechanisms:

- **Session-based** — `middleware_http_auth(ctx)` checks the `session_id` cookie, reads session data via `session_get()`, and loads the user into the context (`httpctx_get_user(ctx)`). Used in the examples below (login, protected route).
- **Token-based** — `authenticate_by_cookie(ctx)` looks the user up directly by the `token` cookie, without involving the session store.
:::

## Usage Examples

### User Registration

```c
#include "http.h"
#include "auth.h"
#include "user.h"

void registration(httpctx_t* ctx) {
    // Read data from the request body
    const char* email = ctx->request->get_payloadf(ctx->request, "email");
    if (email == NULL) {
        ctx->response->status_code = 400;
        ctx->response->send_data(ctx->response, "Email is required");
        return;
    }

    const char* password = ctx->request->get_payloadf(ctx->request, "password");
    if (password == NULL) {
        ctx->response->status_code = 400;
        ctx->response->send_data(ctx->response, "Password is required");
        return;
    }

    const char* name = ctx->request->get_payloadf(ctx->request, "name");

    // Validation
    if (!validate_email(email)) {
        ctx->response->status_code = 400;
        ctx->response->send_data(ctx->response, "Invalid email");
        return;
    }

    if (!validate_password(password)) {
        ctx->response->status_code = 400;
        ctx->response->send_data(ctx->response, "Invalid password");
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
    const char* email = ctx->request->get_payloadf(ctx->request, "email");
    const char* password = ctx->request->get_payloadf(ctx->request, "password");

    if (email == NULL || password == NULL) {
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

    // Create session: session_create(<name>, <data>, <ttl in seconds>)
    json_doc_t* doc = json_root_create_object();
    json_token_t* root = json_root(doc);
    json_object_set(root, "user_id", json_create_number(user_id(user)));

    char* session_id = session_create("backend", json_stringify(doc), 86400);
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
        .same_site = "Lax"
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
        // session_destroy(<name>, <session id>)
        session_destroy("backend", session_id);
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
#include "http.h"
#include "auth.h"
#include "user.h"

void profile(httpctx_t* ctx) {
    // Check authorization via middleware.
    // On success the user is available via httpctx_get_user(ctx).
    middleware(middleware_http_auth(ctx));

    user_t* user = httpctx_get_user(ctx);

    // send_model serializes the model to JSON.
    // display_fields(...) lists the fields included in the response,
    // so secret is guaranteed to be excluded.
    ctx->response->send_model(ctx->response, user, display_fields("id", "email", "name"));
}
```

## Role-Based Access Control (RBAC)

The `role`, `permission`, `role_permission`, and `user_role` models ship with the application (`app/models/`). Below is the relationship schema and SQL-level check examples.

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

Query parameters are passed by name (`:name`) and assembled with the `mparam_*` macros.

```c
int user_has_role(user_t* user, const char* role_name) {
    array_t* params = array_create();
    mparams_fill_array(params,
        mparam_int(user_id, user_id(user)),
        mparam_varchar(role_name, role_name)
    );

    dbresult_t* result = dbquery(
        "postgresql.p1",
        "SELECT 1 FROM user_role ur "
        "JOIN role r ON r.id = ur.role_id "
        "WHERE ur.user_id = :user_id AND r.name = :role_name",
        params
    );

    array_free(params);

    int has_role = (result != NULL && dbresult_query_rows(result) > 0);
    dbresult_free(result);

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

    dbresult_t* result = dbquery(
        "postgresql.p1",
        "SELECT 1 FROM user_role ur "
        "JOIN role_permission rp ON rp.role_id = ur.role_id "
        "JOIN permission p ON p.id = rp.permission_id "
        "WHERE ur.user_id = :user_id AND p.name = :permission_name",
        params
    );

    array_free(params);

    int has_permission = (result != NULL && dbresult_query_rows(result) > 0);
    dbresult_free(result);

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

Checks that an email address is valid: length up to 254 characters, exactly one `@`, a valid local part (up to 64 characters, RFC 5322 allowed characters, no leading/trailing or consecutive dots) and domain (valid characters, at least one dot, a letter-only TLD of at least 2 characters).

**Return Value**\
1 if the address is valid, 0 otherwise.

### Password Validation

```c
int validate_password(const char* password);
```

Checks that a password meets the security requirements:

- length from 8 to 128 characters;
- at least one uppercase letter, one lowercase letter, one digit, and one special character;
- no consecutively repeated characters;
- no common patterns (`123`, `abc`, `qwe`, `password`, …) or keyboard sequences (`qwerty`, `asdfgh`, …) — including reversed.

**Return Value**\
1 if the password is strong, 0 otherwise.

## Constants

```c
#define SALT_SIZE 16      // salt size in bytes
#define HASH_SIZE 32      // hash size in bytes
#define ITERATIONS 140000 // number of PBKDF2 iterations
```
