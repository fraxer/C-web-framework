---
outline: deep
title: WebSockets broadcast
description: Broadcasting messages via the WebSockets protocol
---

# Broadcasting

The ability to transmit real-time data from servers to clients is a requirement for many modern web and mobile applications. When some data is updated on the server, a message is typically sent over a WebSocket connection for processing by the client. WebSockets provide a more efficient alternative to constantly polling your application server for data changes that need to be reflected in your user interface.

The basic concept of broadcasting is simple: clients connect to named pipes on the frontend, while your application broadcasts events to those pipes on the backend. These events can contain any additional data that you want to make available on the front end.

## Channel

A channel is a group of connections/clients for receiving/sending messages.

The channel structure contains:
* channel name
* connection list

Each connection is assigned:
* identification structure
* response handler

An identity structure is needed to send a message to specific clients in a channel.\
The response handler generates the message body before sending it.

## Route configuration

```json
"servers": {
    "s1": {
        ...
        "websockets": {
            "default": ["/var/www/server/build/exec/handlers/libwsindexpage.so", "default_"],
            "routes": {
                "/channel-join": {
                    "GET": ["/var/www/server/build/exec/handlers/libwsindexpage.so", "channel_join"]
                },
                "/channel-leave": {
                    "GET": ["/var/www/server/build/exec/handlers/libwsindexpage.so", "channel_leave"]
                },
                "/channel-send-message": {
                    "POST": ["/var/www/server/build/exec/handlers/libwsindexpage.so", "channel_send"]
                }
            }
        },
        ...
    }
}
```

## Server

### Channel
The `broadcast_add` method creates a channel and adds a connection pointer, connection identification structure data, and a response handler function to it.\
If messages in the channel will be sent to all clients, then there is no need to create a connection identification structure.

```C
void channel_join(wsctx_t* ctx) {
    broadcast_id_t* id = NULL;
    broadcast_add("my_broadcast_name", ctx->request->connection, id, mybroadcast_send_data);
    ctx->response->send_data(ctx->response, "done");
}
```

The `broadcast_remove` method removes the client from the channel.

```C
void channel_leave(wsctx_t* ctx) {
    broadcast_remove("my_broadcast_name", ctx->request->connection);
    ctx->response->send_data(ctx->response, "done");
}
```

### Response handler

Handler for generating the response structure `websocketsresponse_t`.

```C
void mybroadcast_send_data(response_t* response, const char* data, size_t size) {
    websocketsresponse_t* wsresponse = (websocketsresponse_t*)response;
    wsresponse->textn(wsresponse, data, size);
}
```

### Identification structure

Basic connection identification structure:

```C
typedef struct broadcast_id {
    void(*free)(struct broadcast_id*);
} broadcast_id_t;
```

The structure must be allocated memory from the heap so that the scheduler can correctly identify connections.\
Memory is released automatically when the connection is closed or the client leaves the channel.\
To free memory, the structure contains a `free` callback. For each structure, it is necessary to implement its own callback to correctly free memory.

Based on the basic structure, you need to form your own structure.

```C
// broadcasting/mybroadcast.h
typedef struct mybroadcast_id {
    broadcast_id_t base;
    int user_id;
    int project_id;
} mybroadcast_id_t;

mybroadcast_id_t* mybroadcast_id_create();
void mybroadcast_id_free(void*);

// broadcasting/mybroadcast.c
mybroadcast_id_t* mybroadcast_id_create() {
    mybroadcast_id_t* st = malloc(sizeof * st);
    if (st == NULL) return NULL;

    st->base.free = mybroadcast_id_free;
    st->user_id = 1;
    st->project_id = 2;

    return st;
}

void mybroadcast_id_free(void* id) {
    mybroadcast_id_t* my_id = id;
    if (id == NULL) return;

    my_id->user_id = 0;
    my_id->project_id = 0;

    free(my_id);
}
```

Let's create a channel and attach an identification structure to the connection.

```C
void channel_join(wsctx_t* ctx) {
    broadcast_id_t* id = mybroadcast_id_create();
    if (id == NULL) {
        ctx->response->send_data(ctx->response, "out of memory");
        return;
    }

    broadcast_add("my_broadcast_name", ctx->request->connection, id, mybroadcast_send_data);
    ctx->response->send_data(ctx->response, "done");
}
```

### Sending messages

To send data, the `broadcast_send_all` and `broadcast_send` methods are used.
The `broadcast_send_all` method broadcasts data to all recipients.

```C
void channel_send(wsctx_t* ctx) {
    const char* data = "text data";
    size_t length = strlen(data);

    broadcast_send_all("my_broadcast_name", ctx->request->connection, data, length);
    ctx->response->send_data(ctx->response, "done");
}
```

The `broadcast_send` method broadcasts data to recipients by comparing identity structures.

```C
mybroadcast_id_t* mybroadcast_compare_id_create() {
    mybroadcast_id_t* st = malloc(sizeof * st);
    if (st == NULL) return NULL;

    st->base.free = mybroadcast_id_free;
    st->user_id = 1;
    st->project_id = 0;

    return st;
}

int mybroadcast_compare(void* sourceStruct, void* targetStruct) {
    mybroadcast_id_t* sd = sourceStruct;
    mybroadcast_id_t* td = targetStruct;

    return sd->user_id == td->user_id;
}

void channel_send(wsctx_t* ctx) {
    const char* data = "text data";
    size_t length = strlen(data);
    mybroadcast_id_t* id = mybroadcast_compare_id_create();
    if (id == NULL) {
        ctx->response->send_data(ctx->response, "out of memory");
        return;
    }

    broadcast_send("my_broadcast_name", ctx->request->connection, data, length, id, mybroadcast_compare);
    ctx->response->send_data(ctx->response, "done");
}
```

## Client

### Establishing a connection

Connect to the server and send a request to the `channel-join` resource to join the channel.

```js
let socket = new WebSocket("wss://cwebframework.tech/wss", "resource");
socket.onopen = (event) => {
    socket.send("GET /channel-join")
}
socket.onmessage = (event) => {
    console.log(event.data)
}
```

You will now receive messages coming into the channel.

### Sending a message

To send a message, you must send a request to the resource\
`channel-send-message` with payload.

```js
socket.send("POST /channel-send-message new message")
```

The client that sends the message will not receive the message from the channel.