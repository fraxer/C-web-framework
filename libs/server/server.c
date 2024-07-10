#define _GNU_SOURCE
#include <stddef.h>
#include <string.h>
#include <pthread.h>

#include "log.h"
#include "server.h"

void broadcast_free(struct broadcast* broadcast);


server_t* server_create() {
    server_t* server = malloc(sizeof * server);
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

void servers_free(server_t* server) {
    while (server != NULL) {
        server_t* next = server->next;

        server->port = 0;
        server->root_length = 0;

        if (server->domain) domains_free(server->domain);
        server->domain = NULL;

        server->ip = 0;

        if (server->root) free(server->root);
        server->root = NULL;
        
        if (server->index) server_index_destroy(server->index);
        server->index = NULL;

        if (server->http.redirect) redirect_free(server->http.redirect);
        server->http.redirect = NULL;

        if (server->http.route) routes_free(server->http.route);
        server->http.route = NULL;

        if (server->websockets.route) routes_free(server->websockets.route);
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

index_t* server_index_create(const char* value) {
    index_t* result = NULL;
    index_t* index = malloc(sizeof * index);
    if (index == NULL) {
        log_error("server_index_create: alloc memory for index failed\n");
        goto failed;
    }

    index->value = NULL;
    index->length = 0;

    const size_t length = strlen(value);
    if (length == 0) {
        log_error("server_index_create: index value is empty\n");
        goto failed;
    }

    index->value = malloc(length + 1);
    if (index->value == NULL) {
        log_error("server_index_create: alloc memory for index value failed\n");
        goto failed;
    }

    strcpy(index->value, value);
    index->length = length;

    result = index;

    failed:

    if (result == NULL)
        server_index_destroy(index);

    return result;
}

void server_index_destroy(index_t* index) {
    if (index == NULL) return;

    if (index->value != NULL)
        free(index->value);

    free(index);
}

server_chain_t* server_chain_create(server_t* server, routeloader_lib_t* lib) {
    server_chain_t* chain = malloc(sizeof * chain);
    if (chain == NULL) return NULL;

    pthread_mutex_init(&chain->mutex, NULL);
    chain->server = server;
    chain->routeloader = lib;

    return chain;
}

void server_chain_destroy(server_chain_t* server_chain) {
    if (server_chain == NULL) return;

    pthread_mutex_destroy(&server_chain->mutex);
    servers_free(server_chain->server);
    routeloader_free(server_chain->routeloader);

    free(server_chain);
}
