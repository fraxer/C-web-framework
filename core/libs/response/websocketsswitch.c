#include "websocketsswitch.h"

void switch_to_websockets(httpctx_t* ctx) {
    const http1_header_t* connection  = ctx->request->headern(ctx->request, "Connection", 10);
    const http1_header_t* upgrade     = ctx->request->headern(ctx->request, "Upgrade", 7);
    const http1_header_t* ws_version  = ctx->request->headern(ctx->request, "Sec-WebSocket-Version", 21);
    const http1_header_t* ws_key      = ctx->request->headern(ctx->request, "Sec-WebSocket-Key", 17);
    const http1_header_t* ws_protocol = ctx->request->headern(ctx->request, "Sec-WebSocket-Protocol", 22);

    if (connection == NULL || upgrade == NULL || ws_version == NULL || ws_key == NULL) {
        ctx->response->data(ctx->response, "error connect to web socket");
        return;
    }

    char key[128] = {0};
    const char* magic_string = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    strcpy(key, ws_key->value);
    strcat(key, magic_string);

    unsigned char result[40];
    sha1((const unsigned char*)key, strlen(key), result);

    char base64_string[base64_encode_len(20)];
    int retlen = base64_encode(base64_string, (const char*)result, 20);

    ctx->response->headern_add(ctx->response, "Upgrade", 7, "websocket", 9);
    ctx->response->headern_add(ctx->response, "Connection", 10, "Upgrade", 7);
    ctx->response->headern_add(ctx->response, "Sec-WebSocket-Accept", 20, base64_string, retlen);
    ctx->response->connection->switch_to_protocol = protmgr_set_websockets_default;

    if (ws_protocol != NULL && strcmp(ws_protocol->value, "resource") == 0) {
        ctx->response->headern_add(ctx->response, "Sec-WebSocket-Protocol", 22, ws_protocol->value, ws_protocol->value_length);
        ctx->response->connection->switch_to_protocol = protmgr_set_websockets_resource;
    }

    ctx->response->status_code = 101;
    ctx->response->connection->keepalive_enabled = 1;
}
