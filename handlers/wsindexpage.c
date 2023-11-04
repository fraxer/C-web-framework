#include "websockets.h"

void echo(websocketsrequest_t* request, websocketsresponse_t* response) {
    websockets_protocol_resource_t* protocol = (websockets_protocol_resource_t*)request->protocol;

    jsondoc_t* document = protocol->payload_json(protocol);
    if (json_ok(document)) {
        response->text(response, json_stringify(document));
        json_free(document);
        return;
    }

    char* data = protocol->payload(protocol);
    if (data) {
        response->text(response, data);
        free(data);
        return;
    }

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
    char* data = protocol->payload(protocol);
    // const char* q = protocol->query(protocol, "myfield");
    // const char* uri = protocol->uri;

    response->text(response, data);

    if (data) free(data);
}

void default_(websocketsrequest_t* request, websocketsresponse_t* response) {
    response->data(response, "default response");
}
