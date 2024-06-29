#ifndef __CONNECTION__
#define __CONNECTION__

#include <stdatomic.h>

#include "socket.h"
#include "server.h"
#include "openssl.h"
#include "request.h"
#include "response.h"
#include "cqueue.h"
#include "gzip.h"

struct mpxapi;
struct connection;

typedef struct connection_queue_item_data {
    void(*free)(void*);
} connection_queue_item_data_t;

typedef struct connection_queue_item {
    void(*free)(struct connection_queue_item*);
    void(*handle)(void*);
    struct connection* connection;
    connection_queue_item_data_t* data;
} connection_queue_item_t;

typedef enum {
    CONNECTION_DEC_RESULT_DESTROY = 0,
    CONNECTION_DEC_RESULT_DECREMENT
} connection_dec_result_e;

typedef struct connection {
    int fd;
    unsigned keepalive_enabled : 1;
    unsigned destroyed : 1;
    in_addr_t ip;
    unsigned short int port;
    atomic_bool locked;
    atomic_bool onwrite;
    atomic_int ref_count;
    gzip_t gzip;
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
    int(*queue_append)(struct connection_queue_item*);
    void(*queue_append_broadcast)(struct connection_queue_item*);
    int(*queue_pop)(struct connection*);
    void(*switch_to_protocol)(struct connection*);
} connection_t;

connection_t* connection_server_create(connection_t*);
connection_t* connection_client_create(const int, const in_addr_t, const short);
connection_t* connection_alloc(int, struct mpxapi*, in_addr_t, unsigned short int);
void connection_free(connection_t*);
void connection_reset(connection_t*);
int connection_lock(connection_t*);
int connection_unlock(connection_t*);
int connection_trylockwrite(connection_t*);
void connection_inc(connection_t*);
connection_dec_result_e connection_dec(connection_t*);

#endif
