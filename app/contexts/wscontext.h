#ifndef __WSCONTEXT__
#define __WSCONTEXT__

#include "websocketsrequest.h"
#include "websocketsresponse.h"
#include "user.h"

typedef struct wsctx {
    websocketsrequest_t* request;
    websocketsresponse_t* response;
    user_t* user;

    void(*free)(struct wsctx* ctx);
} wsctx_t;

wsctx_t* wsctx_create(void* request, void* response);
void wsctx_set_user(wsctx_t* ctx, user_t* user);
void wsctx_free(wsctx_t* ctx);

#endif