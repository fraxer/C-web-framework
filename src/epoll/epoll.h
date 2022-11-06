#ifndef __EPOLL__
#define __EPOLL__

#include <sys/epoll.h>
#include "../socket/socket.h"
#include "../connection/connection.h"

#define EPOLL_MAX_EVENTS 16

#define EPOLL_BUFFER 16384

typedef struct epoll_event epoll_event_t;

typedef struct socket_epoll {
    socket_t base;
    epoll_event_t* event;
} socket_epoll_t;

void epoll_run();

int epoll_init();

int epoll_after_create_connection(connection_t*);

int epoll_after_read_request(connection_t*);

int epoll_after_write_request(connection_t*);

int epoll_connection_close(connection_t*);

socket_epoll_t* epoll_socket_alloc();

int epoll_control_add(connection_t*, uint32_t);

int epoll_control_mod(connection_t*, uint32_t);

int epoll_control_del(connection_t*);

#endif