#ifndef __WSMIDDLEWARES__
#define __WSMIDDLEWARES__

#include "wscontext.h"
#include "middleware.h"

int middleware_ws_query_param_required(wsctx_t* ctx, char** keys, int size);

#endif