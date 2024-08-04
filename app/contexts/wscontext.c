#include "wscontext.h"

wsctx_t* wsctx_create(void* request, void* response) {
    wsctx_t* ctx = malloc(sizeof * ctx);
    if (ctx == NULL) return NULL;

    ctx->request = request;
    ctx->response = response;
    ctx->free = wsctx_free;

    return ctx;
}

void wsctx_free(wsctx_t* ctx) {
    if (ctx == NULL) return;

    free(ctx);
}
