#include <stddef.h>
#include <string.h>
#include "../log/log.h"
#include "../route/route.h"
#include "server.h"
    #include <stdio.h>

static server_chain_t* server_chain = NULL;
static server_chain_t* last_server_chain = NULL;

void* server_init() {
    return NULL;
}

server_t* server_create() {
    server_t* server = server_alloc();

    if (server == NULL) return NULL;

    server->is_deprecated = 0;
    server->port = 0;
    server->domain = NULL;
    server->ip = 0;
    server->root = NULL;
    server->index = NULL;
    server->redirect = NULL;
    server->route = NULL;
    // server->database = NULL;
    server->next = NULL;

    return server;
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

server_chain_t* server_chain_create(server_t* server) {
    server_chain_t* chain = server_chain_alloc();

    if (chain == NULL) return NULL;

    if (server_chain == NULL) {
        server_chain = chain;
    }

    chain->server = server;
    chain->next = NULL;

    return chain;
}

server_chain_t* server_chain_last() {
    return last_server_chain;
}

int server_chain_append(server_t* server) {
    server_chain_t* chain = server_chain_create(server);

    if (chain == NULL) return -1;

    if (last_server_chain) {
        last_server_chain->next = chain;
    }

    last_server_chain = chain;

    server_chain_t* c = server_chain;

    return 0;
}

void server_chain_destroy_first() {
    if (server_chain == NULL) return;

    server_chain_t* first_server_chain = server_chain;

    server_chain = server_chain->next;

    if (server_chain == NULL) last_server_chain = NULL;

    server_free(first_server_chain->server);

    free(first_server_chain);
}