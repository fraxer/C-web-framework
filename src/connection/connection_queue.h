#ifndef __CONNECTION_QUEUE__
#define __CONNECTION_QUEUE__

#include "connection.h"

typedef struct connection_queue connection_queue_t;

void connection_queue_push(connection_t*);

int connection_queue_empty();

connection_t* connection_queue_pop();

void connection_queue_guard_push(connection_t*);

connection_t* connection_queue_guard_pop();

void connection_queue_broadcast();

#endif
