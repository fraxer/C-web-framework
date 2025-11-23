#ifndef __HTTPCONTEXT__
#define __HTTPCONTEXT__

#include "httprequest.h"
#include "httpresponse.h"
#include "user.h"

typedef struct httpctx {
    httprequest_t* request;
    httpresponse_t* response;
    user_t* user;
} httpctx_t;

void httpctx_init(httpctx_t* ctx, void* request, void* response);
void httpctx_set_user(httpctx_t* ctx, user_t* user);
void httpctx_clear(httpctx_t* ctx);

#endif