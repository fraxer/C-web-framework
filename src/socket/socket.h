#ifndef __SOCKET__
#define __SOCKET__

#include <arpa/inet.h>
#include "../epoll/epoll.h"

typedef struct socket {
    unsigned short int port;
    int fd;
    epoll_event_t event;
    struct socket* next;
} socket_t;

int socket_create(int fd, in_addr_t ip, unsigned short int port);

void socket_free();

void socket_reset_internal();

int socket_is_current(epoll_event_t*, int);

#endif
