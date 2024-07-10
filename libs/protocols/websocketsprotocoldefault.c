#include <string.h>

#include "websocketsprotocoldefault.h"
#include "websocketsparser.h"
#include "appconfig.h"

int websocketsrequest_get_default(connection_t*);
void websockets_protocol_default_reset(void*);
void websockets_protocol_default_free(void*);
int websockets_protocol_default_payload_parse(websocketsrequest_t*, char*, size_t);

websockets_protocol_t* websockets_protocol_default_create() {
    websockets_protocol_default_t* protocol = malloc(sizeof * protocol);
    if (protocol == NULL) return NULL;

    protocol->base.payload_parse = websockets_protocol_default_payload_parse;
    protocol->base.get_resource = websocketsrequest_get_default;
    protocol->base.reset = websockets_protocol_default_reset;
    protocol->base.free = websockets_protocol_default_free;
    protocol->payload = (char*(*)(websockets_protocol_default_t*))websocketsrequest_payload;
    protocol->payload_file = (file_content_t(*)(websockets_protocol_default_t*))websocketsrequest_payload_file;
    protocol->payload_json = (jsondoc_t*(*)(websockets_protocol_default_t*))websocketsrequest_payload_json;

    websockets_protocol_init_payload((websockets_protocol_t*)protocol);

    return (websockets_protocol_t*)protocol;
}

int websocketsrequest_get_default(connection_t* connection) {
    if (!websockets_queue_handler_add(connection, connection->server->websockets.default_handler))
        return 0;

    return 1;
}

void websockets_protocol_default_reset(void* protocol) {
    (void)protocol;
}

void websockets_protocol_default_free(void* protocol) {
    websockets_protocol_default_reset(protocol);
    free(protocol);
}

int websockets_protocol_default_payload_parse(websocketsrequest_t* request, char* string, size_t length) {
    websocketsparser_t* parser = (websocketsparser_t*)request->parser;

    for (size_t i = 0; i < length; i++)
        string[i] ^= parser->frame.mask[parser->payload_index++ % 4];

    const char* tmp_dir = env()->main.tmp;
    if (!websockets_create_tmpfile(request->protocol, tmp_dir))
        return 0;

    off_t payloadlength = lseek(request->protocol->payload.fd, 0, SEEK_END);
    if (payloadlength + length > env()->main.client_max_body_size)
        return 0;

    int r = write(request->protocol->payload.fd, string, length);
    lseek(request->protocol->payload.fd, 0, SEEK_SET);
    if (r <= 0) return 0;
    
    return 1;
}
