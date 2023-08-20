#include "websockets.h"

void echo(websocketsrequest_t* request, websocketsresponse_t* response) {
    websockets_protocol_resource_t* protocol = (websockets_protocol_resource_t*)request->protocol;

    const char* q = protocol->query(protocol, "my");
    if (q) {
        response->text(response, q);
        return;
    }

    if (protocol->uri_length) {
        response->text(response, protocol->uri);
        return;
    }

    response->text(response, "const echo");
}

void test(websocketsrequest_t* request, websocketsresponse_t* response) {
    websockets_protocol_resource_t* protocol = (websockets_protocol_resource_t*)request->protocol;
    char* data = protocol->payloadf(protocol, "myfield");
    const char* q = protocol->query(protocol, "myfield");
    const char* uri = protocol->uri;

    response->text(response, data);

    if (data) free(data);
}

void default_(websocketsrequest_t* request, websocketsresponse_t* response) {
    if (request->type == WEBSOCKETS_TEXT) {
        response->text(response, "default response");
        return;
    }

    response->binary(response, "default response");
}
