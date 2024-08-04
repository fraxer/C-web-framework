#ifndef __HTTPCONTEXT__
#define __HTTPCONTEXT__

#include "http1request.h"
#include "http1response.h"

typedef struct httpctx {
    http1request_t* request;
    http1response_t* response;

    void(*free)(struct httpctx* ctx);
} httpctx_t;

httpctx_t* httpctx_create(void* request, void* response);
void httpctx_free(httpctx_t* ctx);

#endif