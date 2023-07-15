#ifndef __SOCKET__
#define __SOCKET__

#include <arpa/inet.h>

typedef struct socket {
    int fd;
    in_addr_t ip;
    unsigned short int port;
    struct socket* next;
} socket_t;

socket_t* socket_listen_create(in_addr_t, unsigned short int, void*(*)());

void socket_free(socket_t*);

socket_t* socket_find(int, socket_t*);

int socket_set_nonblocking(int);

int socket_set_keepalive(int);

#endif
