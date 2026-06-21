---
outline: deep
description: Session management in C Web Framework. File, Redis or database storage, creating, reading, updating and deleting sessions.
---

# Sessions

Sessions allow you to persist user state between HTTP requests. The framework supports three storage drivers: file system, Redis, and database.

## Configuration

Session settings are specified in the `config.json` file. The `sessions` section is a named map of session configurations, where each key is a unique name used to reference the configuration in code:

```json
{
    "sessions": {
        "default": {
            "driver": "filesystem",
            "storage_name": "sessions",
            "secret": "my-secret-passphrase"
        }
    }
}
```

**Parameters:**
- `driver` — storage driver: `filesystem`, `redis`, or `database`
- `storage_name` — storage name from the `storages` section (for `filesystem` driver)
- `host_id` — database/Redis server host identifier (for `redis` and `database` drivers)
- `secret` — secret passphrase for session data encryption with AES-256-GCM (required)

### Redis Configuration Example

```json
{
    "databases": {
        "redis": [
            {
                "host_id": "r1",
                "ip": "127.0.0.1",
                "port": 6379,
                "dbindex": 0
            }
        ]
    },
    "sessions": {
        "default": {
            "driver": "redis",
            "host_id": "r1",
            "secret": "my-secret-passphrase"
        }
    }
}
```

### Database Configuration Example

```json
{
    "databases": {
        "postgresql": [
            {
                "host_id": "p1",
                "ip": "127.0.0.1",
                "port": 5432,
                "dbname": "mydb",
                "user": "root",
                "password": ""
            }
        ]
    },
    "sessions": {
        "default": {
            "driver": "database",
            "host_id": "p1",
            "secret": "my-secret-passphrase"
        }
    }
}
```

::: tip The sessions table
The `database` driver stores sessions in a `sessions` table with the columns `session_id`, `data` (encrypted value), and `expired_at`. Create this table in advance, for example via a migration.
:::

## Session API

### Creating a Session

```c
#include "session.h"

char* session_create(const char* key, const char* data, long duration);
```

Creates a new session with the specified data. Before creation, all expired sessions of this configuration are removed automatically.

**Parameters**\
`key` — session configuration name from `config.json`.\
`data` — string with session data (usually JSON).\
`duration` — session lifetime in seconds.

**Return Value**\
Pointer to a string with the session identifier. Memory must be freed with `free()`. Returns `NULL` on failure.

<br>

### Getting Session Data

```c
char* session_get(const char* key, const char* session_id);
```

Retrieves session data by identifier.

**Parameters**\
`key` — session configuration name from `config.json`.\
`session_id` — session identifier.

**Return Value**\
Pointer to a string with session data. Memory must be freed with `free()`. Returns `NULL` if the session is not found.

<br>

### Updating a Session

```c
int session_update(const char* key, const char* session_id, const char* data);
```

Updates data of an existing session.

**Parameters**\
`key` — session configuration name from `config.json`.\
`session_id` — session identifier.\
`data` — new session data.

**Return Value**\
Non-zero value on success, zero on error.

<br>

### Deleting a Session

```c
int session_destroy(const char* key, const char* session_id);
```

Deletes a session.

**Parameters**\
`key` — session configuration name from `config.json`.\
`session_id` — session identifier.

**Return Value**\
Non-zero value on success, zero on error.

<br>

### Generating an Identifier

```c
char* session_create_id();
```

Generates a unique session identifier.

**Return Value**\
Pointer to a string with the identifier. Memory must be freed with `free()`. Returns `NULL` on failure.

## Usage Example

```c
#include "http.h"
#include "session.h"

void login(httpctx_t* ctx) {
    // Create session data
    json_doc_t* doc = json_root_create_object();
    json_token_t* object = json_root(doc);
    json_object_set(object, "user_id", json_create_number(42));
    json_object_set(object, "role", json_create_string("admin"));

    // Create session with 3600 seconds lifetime
    char* session_id = session_create("default", json_stringify(doc), 3600);
    json_free(doc);

    if (session_id == NULL) {
        ctx->response->send_data(ctx->response, "Session creation failed");
        return;
    }

    // Send session_id in cookie
    ctx->response->add_cookie(ctx->response, (cookie_t){
        .name = "session_id",
        .value = session_id,
        .seconds = 3600,
        .path = "/",
        .secure = 1,
        .http_only = 1,
        .same_site = "Lax"
    });

    free(session_id);
    ctx->response->send_data(ctx->response, "Login successful");
}

void profile(httpctx_t* ctx) {
    // Get session_id from cookie
    const char* session_id = ctx->request->get_cookie(ctx->request, "session_id");
    if (session_id == NULL) {
        ctx->response->send_data(ctx->response, "Not authenticated");
        return;
    }

    // Get session data
    char* session_data = session_get("default", session_id);
    if (session_data == NULL) {
        ctx->response->send_data(ctx->response, "Session expired");
        return;
    }

    // Parse data
    json_doc_t* doc = json_parse(session_data);
    free(session_data);

    if (doc == NULL) {
        ctx->response->send_data(ctx->response, "Invalid session data");
        return;
    }

    json_token_t* root = json_root(doc);
    json_token_t* user_id_token = json_object_get(root, "user_id");

    int ok = 0;
    int user_id = json_int(user_id_token, &ok);
    json_free(doc);

    char response[64];
    snprintf(response, sizeof(response), "User ID: %d", user_id);
    ctx->response->send_data(ctx->response, response);
}

void logout(httpctx_t* ctx) {
    const char* session_id = ctx->request->get_cookie(ctx->request, "session_id");
    if (session_id != NULL) {
        session_destroy("default", session_id);
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

## Automatic Cleanup

On every `session_create()` call, the framework first removes all expired sessions of the given configuration — no separate cleanup function is needed. Removal is performed by the storage driver: files are deleted from disk, keys from Redis, rows from the `sessions` table.

Thus, stale data is cleaned up automatically as new sessions are created.
