#ifndef __THREADHANDLER__
#define __THREADHANDLER__

#include "../connection/connection.h"

void* thread_handler(void* arg);

void thread_queue_push(connection_t*);

int thread_handler_run(int);

#endif
