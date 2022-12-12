#ifndef __CONNECTION_QUEUE__
#define __CONNECTION_QUEUE__

#include "connection.h"

typedef struct connection_queue connection_queue_t;

static pthread_cond_t connection_queue_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t connection_queue_mutex = PTHREAD_MUTEX_INITIALIZER;

void connection_queue_push(connection_t* connection);

int connection_queue_empty();

connection_t* connection_queue_pop();

#endif
