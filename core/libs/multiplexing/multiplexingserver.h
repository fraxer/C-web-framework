#ifndef __MULTIPLEXINGSERVER__
#define __MULTIPLEXINGSERVER__

#include <errno.h>
#include <sys/epoll.h>

#include "appconfig.h"
#include "server.h"
#include "multiplexing.h"

int mpxserver_run(appconfig_t* appconfig, void(*thread_worker_threads_pause)(appconfig_t* config));

#endif