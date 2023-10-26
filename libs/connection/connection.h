#ifndef __CONNECTION__
#define __CONNECTION__

#include <stdatomic.h>
#include "socket.h"
#include "server.h"
#include "openssl.h"
#include "request.h"
#include "response.h"

typedef enum connection_state {
    CONNECTION_WAITREAD = 0,
    CONNECTION_READ,
    CONNECTION_WAITWRITE,
    CONNECTION_WRITE,
    CONNECTION_WAITCLOSE
} connection_state_e;

// struct broadcast_conn_queue;

typedef struct connection {
    int fd;
    int basefd;
    atomic_int queue_count;
    int keepalive_enabled;
    int timeout;
    int server_finded;
    in_addr_t ip;
    unsigned short int port;
    atomic_bool locked;
    connection_state_e state;
    int* counter;
    void* apidata;
    void* protocol;
    SSL* ssl;
    server_t* server;
    request_t* request;
    response_t* response;
    // struct broadcast_conn_queue* broadcast_queue;

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

void connection_set_state(connection_t*, int);

int connection_state(connection_t*);

int connection_alive(connection_t*);

#endif
