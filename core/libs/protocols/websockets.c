#include "websockets.h"

void websockets_default_handler(wsctx_t* ctx) {
    if (ctx->request->type == WEBSOCKETS_TEXT) {
        ctx->response->text(ctx->response, "");
        return;
    }

    ctx->response->binary(ctx->response, "");
}