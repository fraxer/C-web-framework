#include <string.h>

#include "httpmiddlewares.h"
#include "session.h"

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

int middleware_http_auth(httpctx_t *ctx) {
    const char* session_id = ctx->request->cookie(ctx->request, "session_id");
    if (session_id == NULL) {
        ctx->response->data(ctx->response, "Session id not found");
        return 0;
    }

    int result = 0;
    jsondoc_t* document = NULL;
    char* session_data = session_get(session_id);
    if (session_data == NULL) {
        ctx->response->data(ctx->response, "Session not found");
        return 0;
    }

    document = json_init();
    if (document == NULL) {
        ctx->response->data(ctx->response, "Json document init error");
        goto failed;
    }
    if (!json_parse(document, session_data)) {
        ctx->response->data(ctx->response, "Session data is not valid json");
        goto failed;
    }

    jsontok_t* object = json_root(document);
    if (!json_is_object(object)) {
        ctx->response->data(ctx->response, "Session data is not object");
        goto failed;
    }
    jsontok_t* token_user_id = json_object_get(object, "user_id");
    if (!json_is_int(token_user_id)) {
        ctx->response->data(ctx->response, "Session data is not valid");
        goto failed;
    }

    const int user_id = json_int(token_user_id);
    if (user_id < 1) {
        ctx->response->data(ctx->response, "User is not valid");
        goto failed;
    }

    mparams_create_array(params,
        mparam_int(id, user_id)
    );
    user_t* user = user_get(params);
    array_free(params);

    if (user == NULL) {
        ctx->response->data(ctx->response, "User not found");
        goto failed;
    }

    httpctx_set_user(ctx, user);

    result = 1;

    failed:

    free(session_data);
    json_free(document);

    return result;
}
