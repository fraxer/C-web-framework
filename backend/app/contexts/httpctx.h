#ifndef __HTTPCTX__
#define __HTTPCTX__

#include "http.h"
#include "user.h"

// Helper macro for app-level code to access user as typed pointer
#define httpctx_get_user(ctx) ((user_t*)((ctx)->user_data))

void httpctx_set_user(httpctx_t* ctx, user_t* user);

#endif