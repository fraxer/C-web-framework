#include "websockets.h"
#include "broadcast.h"
#include "mybroadcast.h"

static const char* broadcast_name = "my_broadcast_name";

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

void default_(websocketsrequest_t* request, websocketsresponse_t* response) {
    (void)request;
    response->data(response, "default response");
}

void channel_join(websocketsrequest_t* request, websocketsresponse_t* response) {
    mybroadcast_id_t* id = mybroadcast_id_create();
    if (id == NULL) {
        response->data(response, "out of memory");
        return;
    }

    broadcast_add(broadcast_name, request->connection, id, mybroadcast_send_data);
    response->data(response, "done");
}

void channel_leave(websocketsrequest_t* request, websocketsresponse_t* response) {
    broadcast_remove(broadcast_name, request->connection);
    response->data(response, "done");
}

void channel_send(websocketsrequest_t* request, websocketsresponse_t* response) {
    websockets_protocol_resource_t* protocol = (websockets_protocol_resource_t*)request->protocol;

    char* data = protocol->payload(protocol);
    if (data == NULL) {
        response->data(response, "empty data");
        return;
    }

    size_t length = strlen(data);
    mybroadcast_id_t* id = mybroadcast_compare_id_create();
    if (id == NULL) {
        response->data(response, "out of memory");
        free(data);
        return;
    }

    broadcast_send(broadcast_name, request->connection, data, length, id, mybroadcast_compare);
    free(data);

    response->data(response, "done");
}
