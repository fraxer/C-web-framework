#include "httpcontext.h"

httpctx_t* httpctx_create(void* request, void* response) {
    httpctx_t* ctx = malloc(sizeof * ctx);
    if (ctx == NULL) return NULL;

    ctx->request = request;
    ctx->response = response;
    ctx->free = httpctx_free;

    return ctx;
}

void httpctx_free(httpctx_t* ctx) {
    if (ctx == NULL) return;

    free(ctx);
}
