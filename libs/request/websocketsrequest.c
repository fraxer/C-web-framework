#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "websocketsrequest.h"
#include "websocketsparser.h"
#include "websocketsprotocoldefault.h"
#include "websocketsprotocolresource.h"

void websocketsrequest_init_payload(websocketsrequest_t*);
int websocketsrequest_init_parser(websocketsrequest_t*);
db_t* websocketsrequest_database_list(websocketsrequest_t*);
void websocketsrequest_payload_free(websockets_payload_t*);
int websocketsrequest_get_default(connection_t*);
int websocketsrequest_get_resource(connection_t*);

void websocketsrequest_init_payload(websocketsrequest_t* request) {
    request->payload.fd = 0;
    request->payload.path = NULL;
    request->payload.payload_offset = 0;
}

int websocketsrequest_init_parser(websocketsrequest_t* request) {
    request->parser = malloc(sizeof(websocketsparser_t));
    if (request->parser == NULL) return 0;

    websocketsparser_init(request->parser);

    return 1;
}

websocketsrequest_t* websocketsrequest_alloc() {
    return malloc(sizeof(websocketsrequest_t));
}

void websocketsrequest_free(void* arg) {
    websocketsrequest_t* request = (websocketsrequest_t*)arg;

    websocketsrequest_reset(request);
    websocketsparser_free(request->parser);
    request->protocol->free(request->protocol);

    free(request);

    request = NULL;
}

websocketsrequest_t* websocketsrequest_create(connection_t* connection) {
    websocketsrequest_t* request = websocketsrequest_alloc();
    if (request == NULL) return NULL;

    request->type = WEBSOCKETS_NONE;
    request->control_type = WEBSOCKETS_NONE;
    request->payload_length = 0;
    request->connection = connection;
    request->database_list = websocketsrequest_database_list;
    request->base.reset = (void(*)(void*))websocketsrequest_reset;
    request->base.free = (void(*)(void*))websocketsrequest_free;

    websocketsrequest_init_payload(request);

    if (!websocketsrequest_init_parser(request)) {
        free(request);
        return NULL;
    }

    return request;
}

void websocketsrequest_reset(websocketsrequest_t* request) {
    if (request->control_type == WEBSOCKETS_NONE) {
        request->type = WEBSOCKETS_NONE;

        request->payload_length = 0;
        request->fragmented = 0;
        request->protocol->reset(request->protocol);

        websocketsrequest_payload_free(&request->payload);
    }

    request->control_type = WEBSOCKETS_NONE;
}

db_t* websocketsrequest_database_list(websocketsrequest_t* request) {
    return request->connection->server->database;
}

void websocketsrequest_payload_free(websockets_payload_t* payload) {
    if (payload->fd <= 0) return;

    close(payload->fd);
    unlink(payload->path);

    payload->fd = 0;

    free(payload->path);
    payload->path = NULL;
}
