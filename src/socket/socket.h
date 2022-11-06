#ifndef __SOCKET__
#define __SOCKET__

#include <arpa/inet.h>

typedef struct socket {
    int fd;
    struct socket* next;
} socket_t;

socket_t* socket_listen_create(int, in_addr_t, unsigned short int, void*(*)());

void socket_free();

void socket_reset_internal();

socket_t* socket_find(int, int);

int socket_set_nonblocking(int socket);

int socket_set_keepalive(int socket);

#endif
