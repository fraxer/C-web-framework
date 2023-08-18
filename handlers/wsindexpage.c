#include "websockets.h"

void echo(websocketsrequest_t* request, websocketsresponse_t* response) {
    const char* data = "const echo";

    // if (request->payload)
    //     data = request->payload;

    // data = request->query(request, "my");

    // if (!data)
    //     data = "none";

    response->text(response, data);
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
