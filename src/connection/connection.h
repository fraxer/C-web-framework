#ifndef __CONNECTION__
#define __CONNECTION__

#include <pthread.h>
#include "../server/server.h"
#include "../openssl/openssl.h"
#include "../request/request.h"
#include "../response/response.h"

typedef struct connection {
    int fd;
    int basefd;
    int ssl_enabled;
    int keepalive_enabled;
    int timeout;
    int* counter;
    void* apidata;
    void* protocol;
    SSL* ssl;
    server_t* server;
    request_t* request;
    response_t* response;

    pthread_mutex_t mutex;

    int(*close)(struct connection*);
    void(*read)(struct connection*, char*, size_t);
    void(*handle)(request_t*, response_t*);
    void(*write)(struct connection*, char*, size_t);
    int(*after_read_request)(struct connection*);
    int(*after_write_request)(struct connection*);
    int(*queue_push)(struct connection*);
    int(*queue_pop)(struct connection*);
} connection_t;

connection_t* connection_create(int, int);

connection_t* connection_alloc(int fd, int basefd);

void connection_free(connection_t*);

int connection_trylock(connection_t*);

int connection_unlock(connection_t*);

#endif
