---
outline: deep
description: Session management in C Web Framework. File or Redis storage, creating, reading, updating and deleting sessions.
---

# Sessions

Sessions allow you to persist user state between HTTP requests. The framework supports two storage drivers: file system and Redis.

## Configuration

Session settings are specified in the `config.json` file:

```json
{
    "sessions": {
        "driver": "storage",
        "storage_name": "sessions",
        "lifetime": 3600
    }
}
```

**Parameters:**
- `driver` — storage driver: `storage` (file system) or `redis`
- `storage_name` — storage name from the `storages` section (for file driver) or Redis server `host_id`
- `lifetime` — session lifetime in seconds

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
        "driver": "redis",
        "storage_name": "r1",
        "lifetime": 3600
    }
}
```

## Session API

### Creating a Session

```c
#include "session.h"

char* session_create(const char* data);
```

Creates a new session with the specified data.

**Parameters**\
`data` — string with session data (usually JSON).

**Return Value**\
Pointer to a string with the session identifier. Memory must be freed with `free()`.

<br>

### Getting Session Data

```c
char* session_get(const char* session_id);
```

Retrieves session data by identifier.

**Parameters**\
`session_id` — session identifier.

**Return Value**\
Pointer to a string with session data. Memory must be freed with `free()`. Returns `NULL` if the session is not found.

<br>

### Updating a Session

```c
int session_update(const char* session_id, const char* data);
```

Updates data of an existing session.

**Parameters**\
`session_id` — session identifier.\
`data` — new session data.

**Return Value**\
Non-zero value on success, zero on error.

<br>

### Deleting a Session

```c
int session_destroy(const char* session_id);
```

Deletes a session.

**Parameters**\
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
Pointer to a string with the identifier. Memory must be freed with `free()`.

## Usage Example

```c
#include "http.h"
#include "session.h"
#include "appconfig.h"

void login(httpctx_t* ctx) {
    // Create session data
    json_doc_t* doc = json_root_create_object();
    json_token_t* object = json_root(doc);
    json_object_set(object, "user_id", json_create_number(42));
    json_object_set(object, "role", json_create_string("admin"));

    // Create session
    char* session_id = session_create(json_stringify(doc));
    json_free(doc);

    if (session_id == NULL) {
        ctx->response->send_data(ctx->response, "Session creation failed");
        return;
    }

    // Send session_id in cookie
    ctx->response->add_cookie(ctx->response, (cookie_t){
        .name = "session_id",
        .value = session_id,
        .seconds = appconfig()->sessionconfig.lifetime,
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
    char* session_data = session_get(session_id);
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

## Automatic Cleanup

```c
void session_remove_expired(void);
```

Removes all expired sessions. It is recommended to call periodically to clean up stale data.
