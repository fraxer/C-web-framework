#define _GNU_SOURCE
#include <stddef.h>
#include <string.h>
#include <pthread.h>
#include "../log/log.h"
#include "../route/route.h"
#include "server.h"
    #include <stdio.h>

static server_chain_t* server_chain = NULL;
static server_chain_t* last_server_chain = NULL;

void* server_init() {
    return NULL;
}

struct hsearch_data* server_domain_hash_table_alloc() {
    struct hsearch_data* table = (struct hsearch_data*)malloc(sizeof(struct hsearch_data));

    if (table == NULL) return NULL;

    memset(table, 0, sizeof(struct hsearch_data));
}

server_t* server_create() {
    server_t* result = NULL;
    server_t* server = server_alloc();

    if (server == NULL) return NULL;

    server->domain_hashes = server_domain_hash_table_alloc();

    if (server->domain_hashes == NULL) goto failed;

    server->port = 0;
    server->domain = NULL;
    
    server->ip = 0;
    server->root = NULL;
    server->index = NULL;
    server->redirect = NULL;
    server->route = NULL;
    // server->database = NULL;
    server->next = NULL;

    result = server;

    failed:

    if (result == NULL) {
        free(server);
    }

    return result;
}

server_t* server_alloc() {
    return (server_t*)malloc(sizeof(server_t));
}

void server_free(server_t* server) {
    while (server) {
        server_t* next = server->next;

        server->port = 0;

        domain_free(server->domain);
        server->domain = NULL;

        hdestroy_r(server->domain_hashes);
        free(server->domain_hashes);
        server->domain_hashes = NULL;

        server->ip = 0;

        free(server->root);
        server->root = NULL;

        free(server->index->value);
        free(server->index);
        server->index = NULL;

        server->redirect = NULL;

        route_free(server->route);
        server->route = NULL;

        // server->database = NULL;
        server->next = NULL;

        free(server);

        server = next;
    }
}

index_t* server_create_index(const char* value) {
    index_t* result = NULL;

    index_t* index = (index_t*)malloc(sizeof(index_t));

    if (index == NULL) goto failed;

    size_t length = strlen(value);

    char* string = (char*)malloc(length + 1);

    if (string == NULL) goto failed;

    index->value = string;
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
    chain->connection_count = 0;
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

    server_chain_t* c = server_chain;

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