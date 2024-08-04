#include "websockets.h"
#include "broadcast.h"
#include "mybroadcast.h"
#include "wsmiddlewares.h"

static const char* broadcast_name = "my_broadcast_name";

void echo(wsctx_t* ctx) {
    websockets_protocol_resource_t* protocol = (websockets_protocol_resource_t*)ctx->request->protocol;

    jsondoc_t* document = protocol->payload_json(protocol);
    if (json_ok(document)) {
        ctx->response->text(ctx->response, json_stringify(document));
        json_free(document);
        return;
    }

    char* data = protocol->payload(protocol);
    if (data) {
        ctx->response->text(ctx->response, data);
        free(data);
        return;
    }

    const char* q = protocol->query(protocol, "my");
    if (q) {
        ctx->response->text(ctx->response, q);
        return;
    }

    if (protocol->uri_length) {
        ctx->response->text(ctx->response, protocol->uri);
        return;
    }

    ctx->response->text(ctx->response, "const echo");
}

void post(wsctx_t* ctx) {
    websockets_protocol_resource_t* protocol = (websockets_protocol_resource_t*)ctx->request->protocol;

    file_content_t payload_content = protocol->payload_file(protocol);
    if (!payload_content.ok) {
        ctx->response->data(ctx->response, "file not found");
        return;
    }

    char* content = payload_content.content(&payload_content);

    ctx->response->text(ctx->response, content);

    free(content);
}

void default_(wsctx_t* ctx) {
    ctx->response->data(ctx->response, "default response");
}

void channel_join(wsctx_t* ctx) {
    mybroadcast_id_t* id = mybroadcast_id_create();
    if (id == NULL) {
        ctx->response->data(ctx->response, "out of memory");
        return;
    }

    broadcast_add(broadcast_name, ctx->request->connection, id, mybroadcast_send_data);
    ctx->response->data(ctx->response, "done");
}

void channel_leave(wsctx_t* ctx) {
    broadcast_remove(broadcast_name, ctx->request->connection);
    ctx->response->data(ctx->response, "done");
}

void channel_send(wsctx_t* ctx) {
    websockets_protocol_resource_t* protocol = (websockets_protocol_resource_t*)ctx->request->protocol;

    char* data = protocol->payload(protocol);
    if (data == NULL) {
        ctx->response->data(ctx->response, "empty data");
        return;
    }

    size_t length = strlen(data);
    mybroadcast_id_t* id = mybroadcast_compare_id_create();
    if (id == NULL) {
        ctx->response->data(ctx->response, "out of memory");
        free(data);
        return;
    }

    broadcast_send(broadcast_name, ctx->request->connection, data, length, id, mybroadcast_compare);
    free(data);

    ctx->response->data(ctx->response, "done");
}

void middleware_test(wsctx_t* ctx) {
    middleware(
        middleware_ws_query_param_required(ctx, args_str("abc"))
    )

    ctx->response->data(ctx->response, "done");
}
