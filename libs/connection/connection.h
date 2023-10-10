#ifndef __CONNECTION__
#define __CONNECTION__

#include <stdatomic.h>
#include "socket.h"
#include "server.h"
#include "openssl.h"
#include "request.h"
#include "response.h"

typedef struct connection {
    int fd;
    int basefd;
    int keepalive_enabled;
    int timeout;
    int server_finded;
    in_addr_t ip;
    unsigned short int port;
    atomic_bool locked;
    int* counter;
    void* apidata;
    void* protocol;
    SSL* ssl;
    server_t* server;
    request_t* request;
    response_t* response;
    void* broadcast_list;

    int(*close)(struct connection*);
    void(*read)(struct connection*, char*, size_t);
    void(*handle)(void*, void*);
    void(*write)(struct connection*, char*, size_t);
    int(*after_read_request)(struct connection*);
    int(*after_write_request)(struct connection*);
    int(*queue_push)(struct connection*);
    int(*queue_pop)(struct connection*);
    void(*switch_to_protocol)(struct connection*);
} connection_t;

connection_t* connection_create(socket_t*, int);

connection_t* connection_alloc(int, int, in_addr_t, unsigned short int);

void connection_free(connection_t*);

void connection_reset(connection_t*);

int connection_trylock(connection_t*);

int connection_lock(connection_t*);

int connection_unlock(connection_t*);

#endif
