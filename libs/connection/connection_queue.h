#ifndef __CONNECTION_QUEUE__
#define __CONNECTION_QUEUE__

#include "connection.h"

typedef struct connection_queue_item_data {
    void(*free)(void*);
} connection_queue_item_data_t;

typedef struct connection_queue_item {
    void(*free)(struct connection_queue_item*);
    void(*handle)(void*);
    connection_t* connection;
    connection_queue_item_data_t* data;
} connection_queue_item_t;

int connection_queue_init();
void connection_queue_guard_append(connection_queue_item_t*);
void connection_queue_guard_prepend(connection_queue_item_t*);
connection_queue_item_t* connection_queue_guard_pop();
void connection_queue_broadcast();
connection_queue_item_t* connection_queue_item_create();

#endif
