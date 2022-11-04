#include <stddef.h>
#include <string.h>
#include "../log/log.h"
#include "../route/route.h"
#include "server.h"

static server_t* first_server;
static server_t* last_server;

void* server_iinit() {
    return NULL;
}

server_t* server_create() {
    server_t* server = server_alloc();

    if (server == NULL) return NULL;

    return server;
}

server_t* server_alloc() {
    server_t* server = (server_t*)malloc(sizeof(server_t));

    if (server == NULL) return NULL;

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

void server_free(server_t* server) {
    server->port = 0;
    server->domain = NULL;
    server->ip = 0;

    free(server->root);
    server->root = NULL;

    server->index = NULL;
    server->redirect = NULL;

    route_free(server->route);
    server->route = NULL;

    // server->database = NULL;
    server->next = NULL;

    free(server);
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