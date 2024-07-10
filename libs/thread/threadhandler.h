#ifndef __THREADHANDLER__
#define __THREADHANDLER__

#include "appconfig.h"

void* thread_handler(void* arg);
int thread_handler_run(appconfig_t* appconfig, int thread_count);
void thread_handlers_wakeup();
void thread_handler_set_threads_pause_cb(void(*thread_handler_threads_pause)(appconfig_t* config));

#endif
