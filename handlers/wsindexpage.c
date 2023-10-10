#include "websockets.h"
#include "broadcast.h"

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



void send_data(response_t* response, const char* data) {
    websocketsresponse_t* wsresponse = (websocketsresponse_t*)response;
    wsresponse->text(wsresponse, data);
}

int check_key(void* sourceData, void* targetData) {
    return sourceData == targetData;
}

void br(websocketsrequest_t* request, websocketsresponse_t* response) {
    // if (!broadcast_exist(broadcast_name))
    //     if (!broadcast_create(broadcast_name))
    //         response->text(response, "Error broadcast create");

    // if (!broadcast_contains(broadcast_name, request->connection))
    //     if (!broadcast_item_add(broadcast_name, request->connection, id))
    //         response->text(response, "Error item add");

    // broadcast_item_remove(broadcast_name, request->connection);

    const char* broadcast_name = "br";
    char* id = (char*)"asd";

    if (broadcast_add(broadcast_name, request->connection, id, sizeof(*id) * strlen(id), send_data))
        response->text(response, "Error broadcast create");

    broadcast_remove(broadcast_name, request->connection);

    broadcast_send_all(broadcast_name, "data");
    broadcast_send(broadcast_name, "data", id, check_key);

    if (request->type == WEBSOCKETS_TEXT)
        response->text(response, "done");
    else
        response->binary(response, "done");
}
