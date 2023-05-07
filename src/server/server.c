#define _GNU_SOURCE
#include <stddef.h>
#include <string.h>
#include <pthread.h>
#include "../log/log.h"
#include "server.h"

static server_chain_t* server_chain = NULL;
static server_chain_t* last_server_chain = NULL;

server_t* server_alloc();
server_chain_t* server_chain_alloc();
server_info_t* server_info_alloc();
gzip_mimetype_t* server_gzip_mimetype_alloc();
void server_gzip_mimetype_free(gzip_mimetype_t*);


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
    server->websockets.route = NULL;
    server->database = NULL;
    server->openssl = NULL;
    server->info = NULL;
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

        // if (server->http.redirect) redirect_free(server->http.redirect);
        server->http.redirect = NULL;

        if (server->http.route) route_free(server->http.route);
        server->http.route = NULL;

        if (server->websockets.route) route_free(server->websockets.route);
        server->websockets.route = NULL;

        if (server->database) db_free(server->database);
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
    return malloc(sizeof(server_info_t));
}

server_info_t* server_info_create() {
    server_info_t* info = malloc(sizeof(server_info_t));

    if (info == NULL) return NULL;

    info->read_buffer = 0;
    info->client_max_body_size = 0;
    info->environment = ENV_DEV;
    info->tmp_dir = NULL;
    info->gzip_mimetype = NULL;

    return info;
}

void server_info_free(server_info_t* server_info) {
    if (server_info == NULL) return;

    if (server_info->tmp_dir) free(server_info->tmp_dir);
    server_info->tmp_dir = NULL;

    server_gzip_mimetype_free(server_info->gzip_mimetype);
    server_info->gzip_mimetype = NULL;

    free(server_info);

    server_info = NULL;
}

gzip_mimetype_t* server_gzip_mimetype_alloc() {
    return malloc(sizeof(gzip_mimetype_t));
}

gzip_mimetype_t* server_gzip_mimetype_create(const char* string) {
    gzip_mimetype_t* gzip_mimetype = server_gzip_mimetype_alloc();
    if (gzip_mimetype == NULL) return NULL;

    gzip_mimetype->length = strlen(string);
    gzip_mimetype->value = malloc(gzip_mimetype->length + 1);
    if (gzip_mimetype->value == NULL) {
        free(gzip_mimetype);
        return NULL;
    }

    strcpy(gzip_mimetype->value, string);

    return gzip_mimetype;
}

void server_gzip_mimetype_free(gzip_mimetype_t* gzip_mimetype) {
    while (gzip_mimetype) {
        gzip_mimetype_t* next = gzip_mimetype->next;

        if (gzip_mimetype->value) free(gzip_mimetype->value);

        free(gzip_mimetype);

        gzip_mimetype = next;
    }
}