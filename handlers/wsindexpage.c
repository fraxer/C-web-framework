#include "websockets.h"

void echo(websocketsrequest_t* request, websocketsresponse_t* response) {
    const char* data = "";

    // if (request->payload)
    //     data = request->payload;

    // data = request->query(request, "my");

    // if (!data)
    //     data = "none";

    response->text(response, data);
}

void test(websocketsrequest_t* request, websocketsresponse_t* response) {
    const char* data = "";

    // char* data = ((websockets_protocol_default_t*)request->protocol)->payload();
    // free(data);

    // websockets_protocol_resource_t* protocol = ((websockets_protocol_resource_t*)request->protocol);
    // char* data = protocol->payloadf(protocol, "myfield");
    // char* data = protocol->query(protocol, "myfield");
    // const char* data = protocol->uri;
    // free(data);

    response->text(response, data);
}
