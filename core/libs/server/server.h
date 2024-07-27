#ifndef __SERVER__
#define __SERVER__

#include <arpa/inet.h>
#include <stdatomic.h>

#include "redirect.h"
#include "route.h"
#include "routeloader.h"
#include "domain.h"
#include "openssl.h"

typedef struct index {
    char* value;
    int length;
} index_t;

typedef struct server_http {
    route_t* route;
    redirect_t* redirect;
} server_http_t;

typedef struct server_websockets {
    route_t* route;
    void(*default_handler)(void*, void*);
} server_websockets_t;

struct broadcast;

typedef struct server {
    unsigned short int port;
    size_t root_length;
    in_addr_t ip;
    server_http_t http;
    server_websockets_t websockets;

    char* root;
    domain_t* domain;
    index_t* index;
    openssl_t* openssl;
    struct broadcast* broadcast;
    struct server* next;
} server_t;

typedef struct server_chain {
    pthread_mutex_t mutex;
    server_t* server;
    routeloader_lib_t* routeloader;
} server_chain_t;

server_t* server_create();
index_t* server_index_create(const char*);
void server_index_destroy(index_t*);
void servers_free(server_t* server);

server_chain_t* server_chain_create(server_t* server, routeloader_lib_t*);
void server_chain_destroy(server_chain_t*);

#endif
