#define _GNU_SOURCE
#include <stddef.h>
#include <string.h>
#include <pthread.h>

#include "log.h"
#include "server.h"

static server_chain_t* server_chain = NULL;
static server_chain_t* last_server_chain = NULL;

server_t* server_alloc();
server_chain_t* server_chain_alloc();
void broadcast_free(struct broadcast* broadcast);


server_t* server_create() {
    server_t* server = server_alloc();

    if (server == NULL) return NULL;

    server->port = 0;
    server->root_length = 0;
    server->domain = NULL;
    
    server->ip = 0;
    server->root = NULL;
    server->index = NULL;
    server->http.route = NULL;
    server->http.redirect = NULL;
    server->websockets.default_handler = NULL;
    server->websockets.route = NULL;
    server->openssl = NULL;
    server->broadcast = NULL;
    server->next = NULL;

    return server;
}

server_t* server_alloc() {
    return malloc(sizeof(server_t));
}

void server_free(server_t* server) {
    while (server) {
        server_t* next = server->next;

        server->port = 0;
        server->root_length = 0;

        if (server->domain) domain_free(server->domain);
        server->domain = NULL;

        server->ip = 0;

        if (server->root) free(server->root);
        server->root = NULL;

        
        if (server->index) {
            if (server->index->value) {
                free(server->index->value);
            }
            free(server->index);
        }
        server->index = NULL;

        if (server->http.redirect) redirect_free(server->http.redirect);
        server->http.redirect = NULL;

        if (server->http.route) route_free(server->http.route);
        server->http.route = NULL;

        if (server->websockets.route) route_free(server->websockets.route);
        server->websockets.route = NULL;

        if (server->openssl) openssl_free(server->openssl);
        server->openssl = NULL;

        if (server->broadcast) broadcast_free(server->broadcast);
        server->broadcast = NULL;

        server->next = NULL;

        free(server);

        server = next;
    }
}

index_t* server_create_index(const char* value) {
    index_t* result = NULL;

    index_t* index = malloc(sizeof(index_t));

    if (index == NULL) goto failed;

    size_t length = strlen(value);

    index->value = malloc(length + 1);

    if (index->value == NULL) goto failed;

    strcpy(index->value, value);

    index->length = length;

    result = index;

    failed:

    if (result == NULL) {
        if (index != NULL) free(index);
    }

    return result;
}

server_chain_t* server_chain_alloc() {
    return (server_chain_t*)malloc(sizeof(server_chain_t));
}

server_chain_t* server_chain_create(server_t* server, routeloader_lib_t* lib, int is_hard_reload) {
    server_chain_t* chain = server_chain_alloc();

    if (chain == NULL) return NULL;

    if (server_chain == NULL) {
        server_chain = chain;
    }

    chain->is_deprecated = 0;
    chain->is_hard_reload = is_hard_reload;
    chain->thread_count = 0;
    chain->routeloader = lib;
    chain->server = server;
    chain->prev = NULL;
    chain->next = NULL;
    chain->destroy = server_chain_destroy;

    pthread_mutex_init(&chain->mutex, NULL);

    return chain;
}

server_chain_t* server_chain_last() {
    return last_server_chain;
}

int server_chain_append(server_t* server, routeloader_lib_t* lib, int is_hard_reload) {
    server_chain_t* chain = server_chain_create(server, lib, is_hard_reload);

    if (chain == NULL) return -1;

    if (last_server_chain) {
        last_server_chain->next = chain;

        chain->prev = last_server_chain;
    }

    last_server_chain = chain;

    return 0;
}

void server_chain_destroy(server_chain_t* _server_chain) {
    if (_server_chain == NULL) return;

    server_chain_t* chain = _server_chain;

    if (chain->prev) {
        chain->prev = chain->next;
    }

    if (server_chain == _server_chain) {
        server_chain = server_chain->next;
    }

    if (server_chain == NULL) last_server_chain = NULL;

    server_free(_server_chain->server);

    routeloader_free(_server_chain->routeloader);

    pthread_mutex_destroy(&_server_chain->mutex);

    free(_server_chain);
}
