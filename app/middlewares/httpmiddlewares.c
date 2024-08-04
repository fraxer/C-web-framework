#include <string.h>

#include "httpmiddlewares.h"

int middleware_http_forbidden(httpctx_t* ctx) {
    ctx->response->def(ctx->response, 403);

    return 0;
}

int middleware_http_test_header(httpctx_t* ctx) {
    ctx->response->header_add(ctx->response, "X-Test-Header", "test");

    return 1;
}

int middleware_http_query_param_required(httpctx_t *ctx, char **keys, int size) {
    char message[256] = {0};
    for (int i = 0; i < size; i++) {
        const char* param = ctx->request->query(ctx->request, keys[i]);
        if (param == NULL || param[0] == 0) {
            sprintf(message, "param <%s> not found", keys[i]);
            ctx->response->data(ctx->response, message);
            return 0;
        }
    }

    return 1;
}
