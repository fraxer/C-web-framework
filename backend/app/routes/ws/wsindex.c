#include "websockets.h"
#include "broadcast.h"
#include "mybroadcast.h"
#include "wsmiddlewares.h"

static const char* broadcast_name = "my_broadcast_name";

void echo(wsctx_t* ctx) {
    websockets_protocol_resource_t* protocol = (websockets_protocol_resource_t*)ctx->request->protocol;

    json_doc_t* document = protocol->get_payload_json(protocol);
    if (document != NULL) {
        ctx->response->send_text(ctx->response, json_stringify(document));
        json_free(document);
        return;
    }

    char* data = protocol->get_payload(protocol);
    if (data) {
        ctx->response->send_text(ctx->response, data);
        free(data);
        return;
    }

    const char* q = protocol->get_query(protocol, "my");
    if (q) {
        ctx->response->send_text(ctx->response, q);
        return;
    }

    // if (protocol->uri_length) {
    //     ctx->response->send_text(ctx->response, protocol->uri);
    //     return;
    // }

    ctx->response->send_text(ctx->response, "done");
}

void post(wsctx_t* ctx) {
    websockets_protocol_resource_t* protocol = (websockets_protocol_resource_t*)ctx->request->protocol;

    file_content_t payload_content = protocol->get_payload_file(protocol);
    if (!payload_content.ok) {
        ctx->response->send_data(ctx, "file not found");
        return;
    }

    char* content = payload_content.content(&payload_content);

    ctx->response->send_text(ctx->response, content);

    free(content);
}

void default_(wsctx_t* ctx) {
    ctx->response->send_data(ctx, "default response");
}

void channel_join(wsctx_t* ctx) {
    mybroadcast_id_t* id = mybroadcast_id_create();
    if (id == NULL) {
        ctx->response->send_data(ctx, "out of memory");
        return;
    }

    broadcast_add(broadcast_name, ctx->request->connection, id, mybroadcast_send_data);
    ctx->response->send_data(ctx, "done");
}

void channel_leave(wsctx_t* ctx) {
    broadcast_remove(broadcast_name, ctx->request->connection);
    ctx->response->send_data(ctx, "done");
}

void channel_send(wsctx_t* ctx) {
    websockets_protocol_resource_t* protocol = (websockets_protocol_resource_t*)ctx->request->protocol;

    char* data = protocol->get_payload(protocol);
    if (data == NULL) {
        ctx->response->send_data(ctx, "empty data");
        return;
    }

    size_t length = strlen(data);
    mybroadcast_id_t* id = mybroadcast_compare_id_create();
    if (id == NULL) {
        ctx->response->send_data(ctx, "out of memory");
        free(data);
        return;
    }

    broadcast_send(broadcast_name, ctx->request->connection, data, length, id, mybroadcast_compare);
    free(data);

    ctx->response->send_data(ctx, "done");
}

void middleware_test(wsctx_t* ctx) {
    middleware(
        middleware_ws_query_param_required(ctx, args_str("abc"))
    )

    ctx->response->send_data(ctx, "done");
}
