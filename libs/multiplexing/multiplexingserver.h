#ifndef __MULTIPLEXINGSERVER__
#define __MULTIPLEXINGSERVER__

#include <errno.h>
#include <sys/epoll.h>

#include "server.h"
#include "multiplexing.h"

void mpxserver_run(server_chain_t*);

#endif