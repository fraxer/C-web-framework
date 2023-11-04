#ifndef __CONNECTION__
#define __CONNECTION__

#include <stdatomic.h>

#include "socket.h"
#include "server.h"
#include "openssl.h"
#include "request.h"
#include "response.h"
#include "cqueue.h"

struct mpxapi;
struct connection_queue_item;

typedef struct connection {
    int fd;
    int keepalive_enabled;
    in_addr_t ip;
    unsigned short int port;
    atomic_bool locked;
    atomic_bool onwrite;
    struct mpxapi* api;
    SSL* ssl;
    SSL_CTX* ssl_ctx;
    server_t* server;
    void* client;
    request_t* request;
    response_t* response;
    cqueue_t* queue;

    int(*close)(struct connection*);
    void(*read)(struct connection*, char*, size_t);
    void(*write)(struct connection*, char*, size_t);
    int(*after_read_request)(struct connection*);
    int(*after_write_request)(struct connection*);
    int(*queue_prepend)(struct connection_queue_item*);
    int(*queue_append)(struct connection_queue_item*);
    int(*queue_pop)(struct connection*);
    void(*switch_to_protocol)(struct connection*);
} connection_t;

connection_t* connection_server_create(connection_t*);
connection_t* connection_client_create(const int, const in_addr_t, const short);
connection_t* connection_alloc(int, struct mpxapi*, in_addr_t, unsigned short int);
void connection_server_init(connection_t*, int, int, in_addr_t, unsigned short int);
void connection_free(connection_t*);
void connection_reset(connection_t*);
int connection_trylock(connection_t*);
int connection_lock(connection_t*);
int connection_unlock(connection_t*);
int connection_alive(connection_t*);
int connection_trylockwrite(connection_t*);

#endif
