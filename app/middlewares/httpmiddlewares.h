#ifndef __HTTPMIDDLEWARES__
#define __HTTPMIDDLEWARES__

#include "httpcontext.h"
#include "middleware.h"

int middleware_http_forbidden(httpctx_t* ctx);
int middleware_http_test_header(httpctx_t* ctx);
int middleware_http_query_param_required(httpctx_t* ctx, char** keys, int size);
int middleware_http_auth(httpctx_t* ctx);

#endif