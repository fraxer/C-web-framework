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
    const char* broadcast_name = "br";
    char* id = (char*)"asd";

    broadcast_add(broadcast_name, request->connection, id, sizeof(*id) * strlen(id) + 1, send_data);

    if (request->type == WEBSOCKETS_TEXT)
        response->text(response, "done");
    else
        response->binary(response, "done");
}

void br2(websocketsrequest_t* request, websocketsresponse_t* response) {
    const char* broadcast_name = "br2";
    broadcast_add(broadcast_name, request->connection, NULL, 0, send_data);

    if (request->type == WEBSOCKETS_TEXT)
        response->text(response, "done");
    else
        response->binary(response, "done");
}

void sn(websocketsrequest_t* request, websocketsresponse_t* response) {
    const char* broadcast_name = "br";

    char data[200] = {0};
    int len = sprintf(data, "data %d", request->connection->fd);

    broadcast_send_all(broadcast_name, request->connection, data, len);

    if (request->type == WEBSOCKETS_TEXT)
        response->text(response, "done");
    else
        response->binary(response, "done");
}
