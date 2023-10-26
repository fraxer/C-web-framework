#ifndef __BROADCAST__
#define __BROADCAST__

#include <stddef.h>
#include <stdatomic.h>
#include "connection.h"

typedef struct broadcast_item {
    connection_t* connection;
    void* id;
    void(*response_handler)(response_t* response, const char* payload, size_t size);
    atomic_bool locked;
    struct broadcast_item* next;
} broadcast_item_t;

typedef struct broadcast_list {
    char* name;
    atomic_bool locked;
    broadcast_item_t* item;
    broadcast_item_t* item_last;
    struct broadcast_list* next;
} broadcast_list_t;

typedef struct broadcast {
    atomic_bool locked;
    broadcast_attrs_t* broadcast_attrs;
    broadcast_list_t* list;
    broadcast_list_t* list_last;
} broadcast_t;

typedef struct broadcast_queue {
    atomic_bool locked;
    connection_t* connection;
    void(*response_handler)(response_t* response, const char* payload, size_t size);
    char* payload;
    size_t size;
    struct broadcast_queue* next;
} broadcast_queue_t;

// typedef struct broadcast_conn_queue {
//     atomic_bool locked;
//     broadcast_queue_t* queue;
//     struct broadcast_conn_queue* next;
// } broadcast_conn_queue_t;

broadcast_t* broadcast_init(broadcast_attrs_t* attrs);
broadcast_attrs_t* broadcast_attrs_init();
void broadcast_free(broadcast_t* broadcast);
int broadcast_add(const char* broadcast_name, connection_t* connection, void* id, int size_id, void(*response_handler)(response_t* response, const char* payload, size_t size));
void broadcast_remove(const char* broadcast_name, connection_t* connection);
void broadcast_clear(connection_t* connection);

void broadcast_send_all(const char* broadcast_name, connection_t* connection, const char* payload, size_t size);
void broadcast_send(const char* broadcast_name, connection_t* connection, const char* payload, size_t size, void* id, int(*compare_handler)(void* st1, void* st2));
void broadcast_thread(broadcast_attrs_t* server_broadcast);

#endif
