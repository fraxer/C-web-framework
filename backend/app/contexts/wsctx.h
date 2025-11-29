#ifndef __WSCTX__
#define __WSCTX__

#include "ws.h"
#include "user.h"

// Helper macro for app-level code to access user as typed pointer
#define wsctx_get_user(ctx) ((user_t*)((ctx)->user_data))

void wsctx_set_user(wsctx_t* ctx, user_t* user);

#endif