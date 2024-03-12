#ifndef __CONNECTION_QUEUE__
#define __CONNECTION_QUEUE__

#include "connection.h"

int connection_queue_init();
int connection_queue_lock();
int connection_queue_unlock();
void connection_queue_guard_append(connection_queue_item_t*);
connection_t* connection_queue_guard_pop();
void connection_queue_broadcast();
connection_queue_item_t* connection_queue_item_create();
int connection_queue_clear(connection_t*);

#endif
