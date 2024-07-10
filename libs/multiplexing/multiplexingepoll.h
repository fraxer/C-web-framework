#ifndef __MULTIPLEXINGEPOLL__
#define __MULTIPLEXINGEPOLL__

#include <errno.h>
#include <sys/epoll.h>

#include "multiplexing.h"

typedef struct epoll_event epoll_event_t;

typedef struct epoll_config {
    int basefd;
    int timeout;
    char* buffer;
    size_t buffer_size;
} epoll_config_t;

typedef struct mpxapi_epoll {
    mpxapi_t base;
    int fd;
} mpxapi_epoll_t;

mpxapi_epoll_t* mpx_epoll_init();

#endif