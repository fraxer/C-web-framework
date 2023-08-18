#include <string.h>

#include "websocketsprotocoldefault.h"
#include "websocketsparser.h"

int websocketsrequest_get_default(connection_t*);
void websockets_protocol_default_reset(void*);
void websockets_protocol_default_free(void*);
int websockets_protocol_default_payload_parse(websocketsrequest_t*, char*, size_t, int);

websockets_protocol_t* websockets_protocol_default_create() {
    websockets_protocol_default_t* protocol = malloc(sizeof * protocol);
    if (protocol == NULL) return NULL;

    protocol->base.payload_parse = websockets_protocol_default_payload_parse;
    protocol->base.get_resource = websocketsrequest_get_default;
    protocol->base.reset = websockets_protocol_default_reset;
    protocol->base.free = websockets_protocol_default_free;
    protocol->payload = (char*(*)(websockets_protocol_default_t*))websocketsrequest_payload;
    protocol->payloadf = (char*(*)(websockets_protocol_default_t*, const char*))websocketsrequest_payloadf;
    protocol->payload_json = (jsondoc_t*(*)(websockets_protocol_default_t*))websocketsrequest_payload_json;

    return (websockets_protocol_t*)protocol;
}

int websocketsrequest_get_default(connection_t* connection) {
    connection->handle = connection->server->websockets.default_handler;
    connection->queue_push(connection);

    return 1;
}

void websockets_protocol_default_reset(void* protocol) {
    (void)protocol;
}

void websockets_protocol_default_free(void* protocol) {
    websockets_protocol_default_reset(protocol);
    free(protocol);
}

int websockets_protocol_default_payload_parse(websocketsrequest_t* request, char* string, size_t length, int end) {
    websocketsparser_t* parser = (websocketsparser_t*)request->parser;

    for (size_t i = 0; i < length; i++)
        string[i] ^= parser->frame.mask[parser->payload_index++ % 4];

    if (request->payload.fd <= 0) {
        const char* template = "tmp.XXXXXX";
        const char* tmp_dir = request->connection->server->info->tmp_dir;

        size_t path_length = strlen(tmp_dir) + strlen(template) + 2; // "/", "\0"
        request->payload.path = malloc(path_length);
        snprintf(request->payload.path, path_length, "%s/%s", tmp_dir, template);

        request->payload.fd = mkstemp(request->payload.path);
        if (request->payload.fd == -1) return 0;
    }

    off_t payloadlength = lseek(request->payload.fd, 0, SEEK_END);

    if (payloadlength + length > request->connection->server->info->client_max_body_size)
        return 0;

    int r = write(request->payload.fd, string, length);
    lseek(request->payload.fd, 0, SEEK_SET);
    if (r <= 0) return 0;
    
    return 1;
}
