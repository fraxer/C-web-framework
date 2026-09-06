#include "wsctx.h"

/* See httpctx.c -- wsctx_init/wsctx_clear are the core's now. */

void wsctx_set_user(wsctx_t* ctx, user_t* user) {
    wsctx_set_user_data(ctx, user);
}
