#include "httpcontext.h"

void httpctx_init(httpctx_t* ctx, void* request, void* response) {
    ctx->request = request;
    ctx->response = response;
    ctx->user = NULL;
}

void httpctx_set_user(httpctx_t* ctx, user_t* user) {
    ctx->user = user;
}

void httpctx_clear(httpctx_t* ctx) {
    model_free(ctx->user);
}
