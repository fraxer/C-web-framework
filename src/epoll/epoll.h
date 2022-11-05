#ifndef __EPOLL__
#define __EPOLL__

#include <sys/epoll.h>
#include "../socket/socket.h"
#include "../connection/connection.h"

#define EPOLL_MAX_EVENTS 16

typedef struct epoll_event epoll_event_t;

typedef struct socket_epoll {
    socket_t base;
    epoll_event_t event;
} socket_epoll_t;

void epoll_run();

int epoll_init();

int epoll_close();

int epoll_after_create_connection(connection_t*);

socket_epoll_t* epoll_socket_alloc();

#endif