#ifndef __WSCONTEXT__
#define __WSCONTEXT__

#include "websocketsrequest.h"
#include "websocketsresponse.h"

typedef struct wsctx {
    websocketsrequest_t* request;
    websocketsresponse_t* response;

    void(*free)(struct wsctx* ctx);
} wsctx_t;

wsctx_t* wsctx_create(void* request, void* response);
void wsctx_free(wsctx_t* ctx);

#endif