---
outline: deep
title: WebSocket Broadcasting
description: Система broadcasting для WebSocket в C Web Framework. Рассылка сообщений группам клиентов, именованные каналы и фильтрация получателей.
---

# WebSocket Broadcasting

Broadcasting позволяет отправлять сообщения группам клиентов в реальном времени. Это основа для создания чатов, уведомлений и live-обновлений.

## Концепция

- **Канал** — именованная группа WebSocket-соединений
- **Клиент** — соединение, подключённое к каналу
- **Структура идентификации** — данные для фильтрации получателей
- **Обработчик ответа** — функция формирования сообщения

## Конфигурация

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

## Базовое использование

### Подключение к каналу

```c
#include "websockets.h"
#include "broadcast.h"

void channel_join(wsctx_t* ctx) {
    broadcast_add("notifications", ctx->request->connection, NULL, broadcast_send_text);
    ctx->response->send_text(ctx->response, "Joined channel");
}
```

### Отключение от канала

```c
void channel_leave(wsctx_t* ctx) {
    broadcast_remove("notifications", ctx->request->connection);
    ctx->response->send_text(ctx->response, "Left channel");
}
```

### Отправка всем в канале

```c
void channel_send(wsctx_t* ctx) {
    char* message = websocketsrequest_payload(ctx->request->protocol);

    if (!message) {
        ctx->response->send_text(ctx->response, "No message");
        return;
    }

    // Отправка всем, кроме отправителя
    broadcast_send_all("notifications", ctx->request->connection, message, strlen(message));

    free(message);
    ctx->response->send_text(ctx->response, "Message sent");
}
```

## Обработчик ответа

Функция формирования ответа для клиентов канала:

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

## Фильтрация получателей

Для отправки сообщений определённым клиентам используйте структуру идентификации.

### Определение структуры

```c
// broadcasting/chat_broadcast.h
typedef struct chat_broadcast_id {
    broadcast_id_t base;    // Обязательное базовое поле
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

### Подключение с идентификацией

```c
void join_room(wsctx_t* ctx) {
    const char* room_id_str = ctx->request->query(ctx->request, "room");
    const char* user_id_str = ctx->request->query(ctx->request, "user");

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

### Функция сравнения

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

### Отправка с фильтрацией

```c
void send_to_room(wsctx_t* ctx) {
    const char* room_id_str = ctx->request->query(ctx->request, "room");
    char* message = websocketsrequest_payload(ctx->request->protocol);

    if (!message) {
        ctx->response->send_text(ctx->response, "No message");
        return;
    }

    // Создаём фильтр для комнаты
    chat_broadcast_id_t* filter = chat_broadcast_id_create(0, atoi(room_id_str));

    // Отправляем только клиентам в указанной комнате
    broadcast_send("chat", ctx->request->connection, message, strlen(message),
                   (broadcast_id_t*)filter, compare_by_room);

    free(message);
    ctx->response->send_text(ctx->response, "Message sent to room");
}
```

## API Broadcasting

| Функция | Описание |
|---------|----------|
| `broadcast_add(channel, conn, id, handler)` | Добавить клиента в канал |
| `broadcast_remove(channel, conn)` | Удалить клиента из канала |
| `broadcast_send_all(channel, sender, data, len)` | Отправить всем (кроме sender) |
| `broadcast_send(channel, sender, data, len, filter, cmp)` | Отправить с фильтрацией |

## Пример: Чат-комната

### Сервер

```c
#include "websockets.h"
#include "broadcast.h"
#include "json.h"

void ws_join_chat(wsctx_t* ctx) {
    const char* username = ctx->request->query(ctx->request, "username");

    chat_user_t* user = chat_user_create(username);
    broadcast_add("global_chat", ctx->request->connection,
                  (broadcast_id_t*)user, broadcast_send_text);

    // Уведомление о входе
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

### Клиент (JavaScript)

```javascript
const ws = new WebSocket("wss://example.com/wss", "resource");

ws.onopen = () => {
    ws.send("GET /join-chat?username=Alice");
};

ws.onmessage = (event) => {
    const data = JSON.parse(event.data);
    console.log("Message:", data);
};

// Отправка сообщения
function sendMessage(text) {
    ws.send(`POST /chat-message ${JSON.stringify({ text })}`);
}
```

## Важно

- Память структуры идентификации освобождается автоматически при отключении клиента
- Клиент-отправитель не получает своё сообщение при broadcast
- Каналы создаются автоматически при первом `broadcast_add`
- Каналы удаляются автоматически когда последний клиент отключается
