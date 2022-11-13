#ifndef __THREADWORKER__
#define __THREADWORKER__

#include "../server/server.h"

void* thread_worker(void* arg);

int thread_worker_run(int, server_t*);

#endif
