#include "httpctx.h"
#include "httprequest.h"
#include "httpresponse.h"
#include "model.h"
#include "user.h"

void httpctx_init(httpctx_t* ctx, void* request, void* response) {
    ctx->request = request;
    ctx->response = response;
    ctx->user_data = NULL;
}

void httpctx_set_user(httpctx_t* ctx, user_t* user) {
    ctx->user_data = user;
}

void httpctx_clear(httpctx_t* ctx) {
    model_free(ctx->user_data);
}
