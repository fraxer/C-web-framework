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

server_t* server_alloc();

server_chain_t* server_chain_alloc();


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
    server->database = NULL;
    server->openssl = NULL;
    server->info = NULL;
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

        if (server->domain) domain_free(server->domain);
        server->domain = NULL;

        if (server->domain_hashes) {
            hdestroy_r(server->domain_hashes);
            free(server->domain_hashes);
        }

        server->domain_hashes = NULL;

        server->ip = 0;

        if (server->root) free(server->root);
        server->root = NULL;

        if (server->index->value) free(server->index->value);
        if (server->index) free(server->index);
        server->index = NULL;

        server->redirect = NULL;

        if (server->route) route_free(server->route);
        server->route = NULL;

        if (server->database) database_free(server->database);
        server->database = NULL;

        if (server->openssl) openssl_free(server->openssl);
        server->openssl = NULL;

        server->info = NULL;

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

server_chain_t* server_chain_create(server_t* server, routeloader_lib_t* lib, server_info_t* server_info, int is_hard_reload) {
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
    chain->info = server_info;
    chain->prev = NULL;
    chain->next = NULL;
    chain->destroy = server_chain_destroy;

    pthread_mutex_init(&chain->mutex, NULL);

    return chain;
}

server_chain_t* server_chain_last() {
    return last_server_chain;
}

int server_chain_append(server_t* server, routeloader_lib_t* lib, server_info_t* server_info, int is_hard_reload) {
    server_chain_t* chain = server_chain_create(server, lib, server_info, is_hard_reload);

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

    server_info_free(_server_chain->info);

    free(_server_chain);
}

server_info_t* server_info_alloc() {
    return (server_info_t*)malloc(sizeof(server_info_t));
}

void server_info_free(server_info_t* server_info) {
    if (server_info == NULL) return;

    if (server_info->tmp_dir) free(server_info->tmp_dir);
    server_info->tmp_dir = NULL;

    free(server_info);

    server_info = NULL;
}