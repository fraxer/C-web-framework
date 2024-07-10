#ifndef __THREADWORKER__
#define __THREADWORKER__

#include "appconfig.h"
#include "server.h"

void* thread_worker(void* arg);
int thread_worker_run(appconfig_t* appconfig, int thread_count);
void thread_worker_set_threads_pause_cb(void(*thread_worker_threads_pause)(appconfig_t* config));
void thread_worker_set_threads_shutdown_cb(void(*thread_worker_threads_shutdown)(void));

#endif
