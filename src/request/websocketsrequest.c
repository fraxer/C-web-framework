#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "websocketsrequest.h"
    #include <stdio.h>

void websocketsrequest_reset(websocketsrequest_t*);

websocketsrequest_t* websocketsrequest_alloc() {
    return (websocketsrequest_t*)malloc(sizeof(websocketsrequest_t));
}

void websocketsrequest_query_free(websockets_query_t* query) {
    while (query != NULL) {
        websockets_query_t* next = query->next;

        // websockets_query_free(query);

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

    request->type = WEBSOCKETS_NONE;
    request->uri_length = 0;
    request->path_length = 0;
    request->ext_length = 0;
    request->payload_length = 0;
    request->uri = NULL;
    request->path = NULL;
    request->ext = NULL;
    request->payload = NULL;
    request->query = NULL;
    request->last_query = NULL;
    request->connection = connection;
    request->base.reset = (void(*)(void*))websocketsrequest_reset;
    request->base.free = (void(*)(void*))websocketsrequest_free;

    return request;
}

void websocketsrequest_reset(websocketsrequest_t* request) {
    request->type = WEBSOCKETS_NONE;
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

int websocketsrequest_save_payload(websocketsrequest_t* request, const char* string, size_t length) {
    if (request->payload == NULL) {
        request->payload = (char*)string;
        request->payload_length = length;
    }
    else {
        size_t len = request->payload_length + length;

        char* data = (char*)realloc(request->payload, len);

        if (data == NULL) {
            free(request->payload);
            request->payload = NULL;
            return -1;
        }

        memcpy(&data[request->payload_length], string, length);

        request->payload = data;
        request->payload_length = len;
    }

    return 0;
}