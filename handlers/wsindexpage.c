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

void send_data(response_t* response, const char* data, size_t size) {
    websocketsresponse_t* wsresponse = (websocketsresponse_t*)response;
    wsresponse->textn(wsresponse, data, size);
}

int check_key(void* sourceData, void* targetData) {
    return strcmp(sourceData, targetData) == 0;
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

    broadcast_add(broadcast_name, request->connection, id, sizeof(*id) * strlen(id) + 1, send_data);
    // if (!broadcast_add(broadcast_name, request->connection, id, sizeof(*id) * strlen(id), send_data)) {
    //     response->text(response, "Error broadcast create");
    //     return;
    // }

    // broadcast_remove(broadcast_name, request->connection);

    char data[200] = {0};
    int len = sprintf(data, "data %d", request->connection->fd);

    broadcast_send_all(broadcast_name, request->connection, data, len);
    // broadcast_send(broadcast_name, request->connection, "data", strlen("data"), id, check_key);

    if (request->type == WEBSOCKETS_TEXT)
        response->text(response, "done");
    else
        response->binary(response, "done");
}
