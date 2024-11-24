#ifndef __HTTPCONTEXT__
#define __HTTPCONTEXT__

#include "http1request.h"
#include "http1response.h"
#include "user.h"

typedef struct httpctx {
    http1request_t* request;
    http1response_t* response;
    user_t* user;

    void(*free)(struct httpctx* ctx);
} httpctx_t;

httpctx_t* httpctx_create(void* request, void* response);
void httpctx_set_user(httpctx_t* ctx, user_t* user);
void httpctx_free(httpctx_t* ctx);

#endif