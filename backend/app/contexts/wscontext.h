#ifndef __WSCONTEXT__
#define __WSCONTEXT__

#include "websocketsrequest.h"
#include "websocketsresponse.h"
#include "user.h"

typedef struct wsctx {
    websocketsrequest_t* request;
    websocketsresponse_t* response;
    user_t* user;
} wsctx_t;

void wsctx_init(wsctx_t* ctx, void* request, void* response);
void wsctx_set_user(wsctx_t* ctx, user_t* user);
void wsctx_clear(wsctx_t* ctx);

#endif