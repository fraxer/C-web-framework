#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>

#include "helpers.h"
#include "websocketsrequest.h"
#include "websocketsparser.h"

int websocketsrequest_init_parser(websocketsrequest_t*);
db_t* websocketsrequest_database_list(websocketsrequest_t*);
void websocketsrequest_payload_free(websockets_payload_t*);
int websocketsrequest_get_default(connection_t*);
int websocketsrequest_get_resource(connection_t*);

void websockets_protocol_init_payload(websockets_protocol_t* protocol) {
    protocol->payload.fd = 0;
    protocol->payload.path = NULL;
    protocol->payload.payload_offset = 0;
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
    request->can_reset = 1;
    request->connection = connection;
    request->database_list = websocketsrequest_database_list;
    request->base.reset = (void(*)(void*))websocketsrequest_reset;
    request->base.free = (void(*)(void*))websocketsrequest_free;

    if (!websocketsrequest_init_parser(request)) {
        free(request);
        return NULL;
    }

    return request;
}

void websocketsrequest_reset(websocketsrequest_t* request) {
    if (request->can_reset) {
        if (request->type != WEBSOCKETS_PING && request->type != WEBSOCKETS_PONG)
            request->fragmented = 0;

        request->type = WEBSOCKETS_NONE;
        request->protocol->reset(request->protocol);

        websocketsrequest_payload_free(&request->protocol->payload);
        websocketsparser_reset(request->parser);
    }

    request->can_reset = 1;
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

int websockets_create_tmpfile(websockets_protocol_t* protocol, const char* tmp_dir) {
    if (protocol->payload.fd) return 1;

    protocol->payload.path = create_tmppath(tmp_dir);
    if (protocol->payload.path == NULL)
        return 0;

    protocol->payload.fd = mkstemp(protocol->payload.path);
    if (protocol->payload.fd == -1) {
        free(protocol->payload.path);
        protocol->payload.path = NULL;
        return 0;
    }

    return 1;
}

char* websocketsrequest_payload(websockets_protocol_t* protocol) {
    off_t payload_size = lseek(protocol->payload.fd, 0, SEEK_END);
    lseek(protocol->payload.fd, 0, SEEK_SET);

    char* buffer = malloc(payload_size + 1);
    if (buffer == NULL) return NULL;

    int r = read(protocol->payload.fd, buffer, payload_size);
    lseek(protocol->payload.fd, 0, SEEK_SET);

    buffer[payload_size] = 0;

    if (r < 0) {
        free(buffer);
        buffer = NULL;
    }

    return buffer;
}

jsondoc_t* websocketsrequest_payload_json(websockets_protocol_t* protocol) {
    char* payload = websocketsrequest_payload(protocol);
    if (payload == NULL) return NULL;

    jsondoc_t* document = json_init();
    if (!document) goto failed;
    if (json_parse(document, payload) < 0) goto failed;

    failed:

    free(payload);

    return document;
}
