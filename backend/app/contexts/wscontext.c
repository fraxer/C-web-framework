#include "wscontext.h"

void wsctx_init(wsctx_t* ctx, void* request, void* response) {
    ctx->request = request;
    ctx->response = response;
    ctx->user = NULL;
}

void wsctx_set_user(wsctx_t* ctx, user_t* user) {
    ctx->user = user;
}

void wsctx_clear(wsctx_t* ctx) {
    model_free(ctx->user);
}