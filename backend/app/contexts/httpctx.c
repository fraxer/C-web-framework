#include "httpctx.h"

/* httpctx_init/httpctx_clear now live in the core (protocols/http/httpcontext.c).
 * What stays here is the application's typed view of ctx->user_data: the core
 * only knows it as void* plus a destructor registered in app_init(). */

void httpctx_set_user(httpctx_t* ctx, user_t* user) {
    httpctx_set_user_data(ctx, user);
}
