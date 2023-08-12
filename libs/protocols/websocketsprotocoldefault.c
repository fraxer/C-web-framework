#include "websocketsprotocoldefault.h"

int websocketsrequest_get_default(connection_t*);
void websockets_protocol_default_reset(websockets_protocol_default_t*);
void websockets_protocol_default_free(websockets_protocol_default_t*);

websockets_protocol_default_t* websockets_protocol_default_create() {
    websockets_protocol_default_t* protocol = malloc(sizeof * protocol);
    if (protocol == NULL) return NULL;

    protocol->base.payload_parse = websockets_protocol_default_payload_parse;
    protocol->base.get_resource = websocketsrequest_get_default;
    protocol->base.reset = websockets_protocol_default_reset;
    protocol->base.free = websockets_protocol_default_free;
    protocol->payload = NULL;
    protocol->payloadf = NULL;
    protocol->payload_json = NULL;

    return protocol;
}

int websocketsrequest_get_default(connection_t* connection) {
    connection->handle = connection->server->websockets.default_handler;
    connection->queue_push(connection);

    return 1;
}

void websockets_protocol_default_reset(websockets_protocol_default_t* protocol) {}

void websockets_protocol_default_free(websockets_protocol_default_t* protocol) {
    free(protocol);
}

int websockets_protocol_default_payload_parse(websocketsrequest_t* request, const char* string, size_t length, int end) {
    return 0;
}
