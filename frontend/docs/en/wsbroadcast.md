---
outline: deep
title: WebSocket Broadcasting
description: Broadcasting system for WebSocket in C Web Framework. Message distribution to client groups, named channels and recipient filtering.
---

# WebSocket Broadcasting

Broadcasting allows sending messages to groups of clients in real time. This is the foundation for creating chats, notifications, and live updates.

## Concept

- **Channel** — a named group of WebSocket connections
- **Client** — a connection subscribed to a channel
- **Identification structure** — data for filtering recipients
- **Response handler** — function for forming the message

## Configuration

```json
"websockets": {
    "routes": {
        "/channel-join": {
            "GET": { "file": "handlers/libws.so", "function": "channel_join" }
        },
        "/channel-leave": {
            "GET": { "file": "handlers/libws.so", "function": "channel_leave" }
        },
        "/channel-send": {
            "POST": { "file": "handlers/libws.so", "function": "channel_send" }
        }
    }
}
```

## Basic usage

### Joining a channel

```c
#include "websockets.h"
#include "broadcast.h"

void channel_join(wsctx_t* ctx) {
    broadcast_add("notifications", ctx->request->connection, NULL, broadcast_send_text);
    ctx->response->send_text(ctx->response, "Joined channel");
}
```

### Leaving a channel

```c
void channel_leave(wsctx_t* ctx) {
    broadcast_remove("notifications", ctx->request->connection);
    ctx->response->send_text(ctx->response, "Left channel");
}
```

### Sending to everyone in a channel

```c
void channel_send(wsctx_t* ctx) {
    char* message = websocketsrequest_payload(ctx->request->protocol);

    if (!message) {
        ctx->response->send_text(ctx->response, "No message");
        return;
    }

    // Send to everyone except the sender
    broadcast_send_all("notifications", ctx->request->connection, message, strlen(message));

    free(message);
    ctx->response->send_text(ctx->response, "Message sent");
}
```

## Response handler

The handler determines how a subscriber receives the message. It is a user-defined function with the signature `void(*)(response_t*, const char*, size_t)`, called separately for each recipient. Typical implementations send a text or binary frame:

```c
void broadcast_send_text(response_t* response, const char* data, size_t size) {
    websocketsresponse_t* ws_response = (websocketsresponse_t*)response;
    ws_response->send_textn(ws_response, data, size);
}

void broadcast_send_binary(response_t* response, const char* data, size_t size) {
    websocketsresponse_t* ws_response = (websocketsresponse_t*)response;
    ws_response->send_binaryn(ws_response, data, size);
}
```

## Recipient filtering

To send messages to specific clients, use an identification structure. It inherits from `broadcast_id_t` (always as the first field) and plays two roles: when subscribing a client (`broadcast_add`) and as a filter when sending (`broadcast_send`).

::: warning Identifier lifetime
These roles have different lifetimes:
- the identifier from `broadcast_add` is freed through its `free` handler when the subscriber is removed — on client disconnect or `broadcast_remove`;
- the filter identifier from `broadcast_send` **is freed by the function itself** through `free` immediately after distribution — do not free it manually and do not reuse it after the call.
:::

### Defining the structure

```c
// broadcasting/chat_broadcast.h
typedef struct chat_broadcast_id {
    broadcast_id_t base;    // Required base field
    int user_id;
    int room_id;
} chat_broadcast_id_t;

chat_broadcast_id_t* chat_broadcast_id_create(int user_id, int room_id);
void chat_broadcast_id_free(void* id);
```

```c
// broadcasting/chat_broadcast.c
#include "chat_broadcast.h"

chat_broadcast_id_t* chat_broadcast_id_create(int user_id, int room_id) {
    chat_broadcast_id_t* id = malloc(sizeof(chat_broadcast_id_t));
    if (!id) return NULL;

    id->base.free = chat_broadcast_id_free;
    id->user_id = user_id;
    id->room_id = room_id;

    return id;
}

void chat_broadcast_id_free(void* ptr) {
    if (ptr) free(ptr);
}
```

### Connecting with identification

```c
#include "websockets.h"

void join_room(wsctx_t* ctx) {
    websockets_protocol_resource_t* protocol = (websockets_protocol_resource_t*)ctx->request->protocol;
    const char* room_id_str = protocol->get_query(protocol, "room");
    const char* user_id_str = protocol->get_query(protocol, "user");

    if (!room_id_str || !user_id_str) {
        ctx->response->send_text(ctx->response, "Missing parameters");
        return;
    }

    int room_id = atoi(room_id_str);
    int user_id = atoi(user_id_str);

    chat_broadcast_id_t* id = chat_broadcast_id_create(user_id, room_id);
    if (!id) {
        ctx->response->send_text(ctx->response, "Memory error");
        return;
    }

    broadcast_add("chat", ctx->request->connection, (broadcast_id_t*)id, broadcast_send_text);
    ctx->response->send_text(ctx->response, "Joined room");
}
```

### Comparison function

```c
int compare_by_room(void* source, void* target) {
    chat_broadcast_id_t* src = (chat_broadcast_id_t*)source;
    chat_broadcast_id_t* tgt = (chat_broadcast_id_t*)target;

    return src->room_id == tgt->room_id;
}

int compare_by_user(void* source, void* target) {
    chat_broadcast_id_t* src = (chat_broadcast_id_t*)source;
    chat_broadcast_id_t* tgt = (chat_broadcast_id_t*)target;

    return src->user_id == tgt->user_id;
}
```

### Sending with filtering

```c
#include "websockets.h"

void send_to_room(wsctx_t* ctx) {
    websockets_protocol_resource_t* protocol = (websockets_protocol_resource_t*)ctx->request->protocol;
    const char* room_id_str = protocol->get_query(protocol, "room");

    if (!room_id_str) {
        ctx->response->send_text(ctx->response, "Missing room parameter");
        return;
    }

    char* message = websocketsrequest_payload(ctx->request->protocol);

    if (!message) {
        ctx->response->send_text(ctx->response, "No message");
        return;
    }

    // Create filter for the room.
    // Note: broadcast_send frees filter via its free handler — do not free it yourself.
    chat_broadcast_id_t* filter = chat_broadcast_id_create(0, atoi(room_id_str));

    // Send only to clients in the specified room
    broadcast_send("chat", ctx->request->connection, message, strlen(message),
                   (broadcast_id_t*)filter, compare_by_room);

    free(message);
    ctx->response->send_text(ctx->response, "Message sent to room");
}
```

## Broadcasting API

| Function | Description |
|----------|-------------|
| `broadcast_add(channel, conn, id, handler)` | Subscribe a connection to a channel; the channel is created automatically. Returns `1` on success, `0` on error or if the connection is already subscribed |
| `broadcast_remove(channel, conn)` | Unsubscribe a connection from a channel; an empty channel is destroyed |
| `broadcast_clear(conn)` | Unsubscribe a connection from all channels at once (called automatically when the connection closes) |
| `broadcast_send_all(channel, sender, data, len)` | Send to all channel subscribers except the sender |
| `broadcast_send(channel, sender, data, len, filter, cmp)` | Send to subscribers filtered by `filter`. If `filter` or `cmp` is `NULL`, the message goes to all (except the sender). The function takes ownership of `filter` and frees it |

::: tip Filtering applies only to recipients
The comparison runs between the subscriber's identifier (the first argument of `cmp`) and the passed `filter` (the second argument). Every subscriber for which `cmp` returns a non-zero value receives the message; the sender is always excluded.
:::

## Example: Chat room

### Server

```c
#include "websockets.h"
#include "broadcast.h"
#include "json.h"

void ws_join_chat(wsctx_t* ctx) {
    websockets_protocol_resource_t* protocol = (websockets_protocol_resource_t*)ctx->request->protocol;
    const char* username = protocol->get_query(protocol, "username");

    if (!username) {
        ctx->response->send_text(ctx->response, "Missing username parameter");
        return;
    }

    chat_user_t* user = chat_user_create(username);
    broadcast_add("global_chat", ctx->request->connection,
                  (broadcast_id_t*)user, broadcast_send_text);

    // Join notification
    json_doc_t* doc = json_init();
    json_token_t* root = json_create_object(doc);
    json_object_set(root, "type", json_create_string(doc, "user_joined"));
    json_object_set(root, "username", json_create_string(doc, username));

    const char* json = json_stringify(doc);
    broadcast_send_all("global_chat", ctx->request->connection, json, strlen(json));
    json_free(doc);

    ctx->response->send_text(ctx->response, "Welcome to chat");
}

void ws_chat_message(wsctx_t* ctx) {
    char* message = websocketsrequest_payload(ctx->request->protocol);
    if (!message) return;

    broadcast_send_all("global_chat", ctx->request->connection, message, strlen(message));
    free(message);
}
```

### Client (JavaScript)

```javascript
const ws = new WebSocket("wss://example.com/wss", "resource");

ws.onopen = () => {
    ws.send("GET /join-chat?username=Alice");
};

ws.onmessage = (event) => {
    const data = JSON.parse(event.data);
    console.log("Message:", data);
};

// Send message
function sendMessage(text) {
    ws.send(`POST /chat-message ${JSON.stringify({ text })}`);
}
```

## Important

- The identifier from `broadcast_add` is freed automatically when the subscriber is removed (client disconnect or `broadcast_remove`), while the filter identifier from `broadcast_send` is freed by `broadcast_send` itself right after distribution
- A connection can subscribe to a channel only once — a repeated `broadcast_add` returns `0` and does not add a duplicate
- On disconnect, a connection is unsubscribed from all channels via `broadcast_clear`, so no explicit cleanup is needed in a close handler
- The sender never receives its own message
- Channels are created automatically on the first `broadcast_add` and destroyed automatically when the last subscriber leaves
