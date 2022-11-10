#ifndef __SOCKET__
#define __SOCKET__

#include <arpa/inet.h>

typedef struct socket {
    int fd;
    struct socket* next;
} socket_t;

socket_t* socket_listen_create(int, in_addr_t, unsigned short int, void*(*)());

void socket_free();

socket_t* socket_find(int, int, socket_t*);

int socket_set_nonblocking(int);

int socket_set_keepalive(int);

#endif
