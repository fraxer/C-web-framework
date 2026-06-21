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

Обработчик определяет, в каком виде подписчик получит сообщение. Это пользовательская функция с сигнатурой `void(*)(response_t*, const char*, size_t)`, которая вызывается отдельно для каждого получателя. Типовые реализации — отправка текстовым или бинарным кадром:

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

Для отправки сообщений определённым клиентам используйте структуру идентификации. Она наследуется от `broadcast_id_t` (обязательно первым полем) и участвует в двух ролях: при подписке клиента (`broadcast_add`) и как фильтр при отправке (`broadcast_send`).

::: warning Время жизни идентификатора
У этих ролей разное время жизни:
- идентификатор из `broadcast_add` освобождается через заданный `free` при удалении подписчика — при отключении клиента или `broadcast_remove`;
- идентификатор-фильтр из `broadcast_send` **освобождается самой функцией** через `free` сразу после рассылки — не освобождайте его вручную и не переиспользуйте после вызова.
:::

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

    // Создаём фильтр для комнаты.
    // Внимание: broadcast_send сам освободит filter через его free — не делайте это вручную.
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
| `broadcast_add(channel, conn, id, handler)` | Подписать соединение на канал; канал создаётся автоматически. Возвращает `1` при успехе, `0` при ошибке или если соединение уже подписано |
| `broadcast_remove(channel, conn)` | Отписать соединение от канала; пустой канал удаляется |
| `broadcast_clear(conn)` | Отписать соединение сразу от всех каналов (вызывается автоматически при закрытии соединения) |
| `broadcast_send_all(channel, sender, data, len)` | Отправить всем подписчикам канала, кроме отправителя |
| `broadcast_send(channel, sender, data, len, filter, cmp)` | Отправить подписчикам с фильтрацией по `filter`. Если `filter` или `cmp` равны `NULL` — уходит всем (кроме отправителя). Функция забирает владение `filter` и освобождает его |

::: tip Фильтрация применяется только к получателям
Сравнение выполняется между идентификатором подписчика (первый аргумент `cmp`) и переданным `filter` (второй аргумент). Сообщение получает каждый подписчик, для которого `cmp` вернул ненулевое значение; отправитель исключается всегда.
:::

## Пример: Чат-комната

### Сервер

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

- Идентификатор из `broadcast_add` освобождается автоматически при удалении подписчика (отключение клиента или `broadcast_remove`), а идентификатор-фильтр из `broadcast_send` — сразу после рассылки самой функцией `broadcast_send`
- Одно соединение может быть подписано на канал только один раз — повторный `broadcast_add` вернёт `0` и не добавит дубль
- При отключении соединение отписывается от всех каналов через `broadcast_clear`, поэтому явная очистка в обработчике закрытия не требуется
- Клиент-отправитель никогда не получает собственное сообщение
- Каналы создаются автоматически при первом `broadcast_add` и удаляются автоматически, когда уходит последний подписчик
