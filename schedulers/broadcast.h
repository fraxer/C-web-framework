#ifndef __BROADCAST__
#define __BROADCAST__

#include <stddef.h>
#include "connection.h"
#include "server.h"

typedef struct broadcast_item {
    connection_t* connection;
    void* id;
    void(*response_handler)(response_t* response, const char* payload);
    int mutex;
    struct broadcast_item* next;
} broadcast_item_t;

typedef struct broadcast_list {
    char* name;
    int mutex;
    broadcast_item_t* item;
    broadcast_item_t* item_last;
    struct broadcast_list* next;
} broadcast_list_t;

int broadcast_add(const char* broadcast_name, connection_t* connection, void* id, int size_id, void(*response_handler)(response_t* response, const char* payload));
void broadcast_remove(const char* broadcast_name, connection_t* connection);

void broadcast_send_all(const char* broadcast_name, connection_t* connection, const char* payload, );
void broadcast_send(const char* broadcast_name, connection_t* connection, const char* payload, void* id, int(*compare_handler)(void* st1, void* st2));



#endif