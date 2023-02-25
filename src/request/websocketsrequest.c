#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "websocketsrequest.h"

db_t* websocketsrequest_database_list(websocketsrequest_t*);

websocketsrequest_t* websocketsrequest_alloc() {
    return (websocketsrequest_t*)malloc(sizeof(websocketsrequest_t));
}

void websocketsrequest_query_free(websockets_query_t* query) {
    while (query != NULL) {
        websockets_query_t* next = query->next;

        websockets_query_free(query);

        query = next;
    }
}

void websocketsrequest_free(void* arg) {
    websocketsrequest_t* request = (websocketsrequest_t*)arg;

    websocketsrequest_reset(request);

    free(request);

    request = NULL;
}

websocketsrequest_t* websocketsrequest_create(connection_t* connection) {
    websocketsrequest_t* request = websocketsrequest_alloc();

    if (request == NULL) return NULL;

    request->method = ROUTE_NONE;
    request->type = WEBSOCKETS_NONE;
    request->control_type = WEBSOCKETS_NONE;
    request->uri_length = 0;
    request->path_length = 0;
    request->ext_length = 0;
    request->payload_length = 0;
    request->control_payload_length = 0;
    request->uri = NULL;
    request->path = NULL;
    request->ext = NULL;
    request->payload = NULL;
    request->control_payload = NULL;
    request->query = NULL;
    request->last_query = NULL;
    request->connection = connection;
    request->database_list = websocketsrequest_database_list;
    request->base.reset = (void(*)(void*))websocketsrequest_reset;
    request->base.free = (void(*)(void*))websocketsrequest_free;

    return request;
}

void websocketsrequest_reset(websocketsrequest_t* request) {
    if (request->control_type == WEBSOCKETS_NONE) {
        request->type = WEBSOCKETS_NONE;
        request->method = ROUTE_NONE;
        request->uri_length = 0;
        request->path_length = 0;
        request->ext_length = 0;
        request->payload_length = 0;

        if (request->uri) free((void*)request->uri);
        request->uri = NULL;

        if (request->path) free((void*)request->path);
        request->path = NULL;

        if (request->ext) free((void*)request->ext);
        request->ext = NULL;

        if (request->payload) free(request->payload);
        request->payload = NULL;

        websocketsrequest_query_free(request->query);
        request->query = NULL;
        request->last_query = NULL;
    }

    request->control_payload_length = 0;

    if (request->control_payload) free(request->control_payload);
    request->control_payload = NULL;

    request->control_type = WEBSOCKETS_NONE;
}

db_t* websocketsrequest_database_list(websocketsrequest_t* request) {
    return request->connection->server->database;
}
