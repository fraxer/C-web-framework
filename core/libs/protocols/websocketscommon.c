#include <stdlib.h>
#include <string.h>

#include "websocketscommon.h"
#include "wscontext.h"
#include "middleware.h"

websockets_query_t* websockets_query_alloc() {
    return (websockets_query_t*)malloc(sizeof(websockets_query_t));
}

websockets_query_t* websockets_query_create(const char* key, size_t key_length, const char* value, size_t value_length) {
    websockets_query_t* query = websockets_query_alloc();

    if (query == NULL) return NULL;

    query->key = websockets_set_field(key, key_length);
    query->value = websockets_set_field(value, value_length);
    query->next = NULL;

    return query;
}

void websockets_query_free(websockets_query_t* query) {
    free((void*)query->key);
    free((void*)query->value);
    free(query);
}

const char* websockets_set_field(const char* string, size_t length) {
    if (string == NULL) return NULL;

    char* value = (char*)malloc(length + 1);

    if (value == NULL) return value;

    strncpy(value, string, length);

    value[length] = 0;

    return value;
}

void websockets_frame_init(websockets_frame_t* frame) {
    frame->fin = 0;
    frame->rsv1 = 0;
    frame->rsv2 = 0;
    frame->rsv3 = 0;
    frame->opcode = 0;
    frame->masked = 0;
    frame->mask[0] = 0;
    frame->mask[1] = 0;
    frame->mask[2] = 0;
    frame->mask[3] = 0;
    frame->payload_length = 0;
}

int websockets_queue_handler_add(connection_t* connection, void(*handle)(void*)) {
    connection_queue_item_t* item = connection_queue_item_create();
    if (item == NULL) return 0;

    item->handle = websockets_queue_handler;
    item->connection = connection;
    item->data = (connection_queue_item_data_t*)websockets_queue_data_create(connection, handle);

    if (item->data == NULL) {
        item->free(item);
        return 0;
    }

    item->connection->request = NULL;

    connection->queue_append(item);

    return 1;
}

void websockets_queue_handler(void* arg) {
    connection_queue_item_t* item = arg;
    connection_queue_websockets_data_t* data = (connection_queue_websockets_data_t*)item->data;

    item->connection->request = data->request;

    if (run_middlewares(item->connection->server->websockets.middleware, data->ctx))
        data->handler(data->ctx);

    item->connection->queue_pop(item->connection);
}

connection_queue_websockets_data_t* websockets_queue_data_create(connection_t* connection, void(*handle)(void*)) {
    connection_queue_websockets_data_t* data = malloc(sizeof * data);
    if (data == NULL) return NULL;

    wsctx_t* ctx = wsctx_create(connection->request, connection->response);
    if (ctx == NULL) return NULL;

    data->base.free = websockets_queue_data_free;
    data->request = connection->request;
    data->ctx = ctx;
    data->handler = handle;

    return data;
}

void websockets_queue_data_free(void* arg) {
    if (arg == NULL) return;

    connection_queue_websockets_data_t* data = arg;

    if (data->ctx != NULL)
        data->ctx->free(data->ctx);

    free(data);
}
