#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "http1request.h"

void http1request_reset(http1request_t*);
http1_header_t* http1request_header(http1request_t*, const char*);
http1_header_t* http1request_headern(http1request_t*, const char*, size_t);
db_t* http1request_database_list(http1request_t*);

http1request_t* http1request_alloc() {
    return (http1request_t*)malloc(sizeof(http1request_t));
}

void http1request_header_free(http1_header_t* header) {
    while (header) {
        http1_header_t* next = header->next;

        http1_header_free(header);

        header = next;
    }
}

void http1request_query_free(http1_query_t* query) {
    while (query != NULL) {
        http1_query_t* next = query->next;

        http1_query_free(query);

        query = next;
    }
}

void http1request_free(void* arg) {
    http1request_t* request = (http1request_t*)arg;

    http1request_reset(request);

    free(request);

    request = NULL;
}

http1request_t* http1request_create(connection_t* connection) {
    http1request_t* request = http1request_alloc();

    if (request == NULL) return NULL;

    request->method = ROUTE_NONE;
    request->version = HTTP1_VER_NONE;
    request->uri_length = 0;
    request->path_length = 0;
    request->ext_length = 0;
    request->uri = NULL;
    request->path = NULL;
    request->ext = NULL;
    request->payload = NULL;
    request->query = NULL;
    request->last_query = NULL;
    request->header_ = NULL;
    request->last_header = NULL;
    request->connection = connection;
    request->header = http1request_header;
    request->headern = http1request_headern;
    request->database_list = http1request_database_list;
    request->base.reset = (void(*)(void*))http1request_reset;
    request->base.free = (void(*)(void*))http1request_free;

    return request;
}

void http1request_reset(http1request_t* request) {
    request->method = ROUTE_NONE;
    request->version = HTTP1_VER_NONE;
    request->uri_length = 0;
    request->path_length = 0;
    request->ext_length = 0;

    if (request->uri) free((void*)request->uri);
    request->uri = NULL;

    if (request->path) free((void*)request->path);
    request->path = NULL;

    if (request->ext) free((void*)request->ext);
    request->ext = NULL;

    if (request->payload) free(request->payload);
    request->payload = NULL;

    http1request_query_free(request->query);
    request->query = NULL;
    request->last_query = NULL;

    http1request_header_free(request->header_);
    request->header_ = NULL;
    request->last_header = NULL;
}

http1_header_t* http1request_header(http1request_t* request, const char* key) {
    return http1request_headern(request, key, strlen(key));
}

http1_header_t* http1request_headern(http1request_t* request, const char* key, size_t key_length) {
    http1_header_t* header = request->header_;

    while (header) {
        for (size_t i = 0, j = 0; i < header->key_length && j < key_length; i++, j++) {
            if (tolower(header->key[i]) != tolower(key[j])) goto next;
        }

        return header;

        next:

        header = header->next;
    }

    return NULL;
}

db_t* http1request_database_list(http1request_t* request) {
    return request->connection->server->database;
}