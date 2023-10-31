#ifndef __THREADBROADCAST__
#define __THREADBROADCAST__

#include "broadcast.h"

void* thread_broadcast(void* arg);

int thread_broadcast_run(int, server_chain_t*);

#endif
