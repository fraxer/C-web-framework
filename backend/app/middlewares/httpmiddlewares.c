#include <string.h>
#include <arpa/inet.h>

#include "httpmiddlewares.h"
#include "session.h"
#include "query.h"
#include "log.h"

int middleware_http_forbidden(httpctx_t* ctx) {
    ctx->response->send_default(ctx->response, 403);

    return 0;
}

int middleware_http_test_header(httpctx_t* ctx) {
    ctx->response->add_header(ctx->response, "X-Test-Header", "test");

    return 1;
}

int middleware_http_query_param_required(httpctx_t *ctx, char **keys, int size) {
    char message[256] = {0};
    int ok = 0;
    for (int i = 0; i < size; i++) {
        const char* param = query_param_char(ctx->request, keys[i], &ok);
        if (!ok)
            return 0;

        if (param == NULL || param[0] == 0) {
            sprintf(message, "param \"%s\" not found", keys[i]);
            ctx->response->send_data(ctx->response, message);
            return 0;
        }
    }

    return 1;
}

int middleware_http_auth(httpctx_t *ctx) {
    const char* session_id = ctx->request->get_cookie(ctx->request, "session_id");
    if (session_id == NULL) {
        ctx->response->send_data(ctx->response, "Session id not found");
        return 0;
    }

    int result = 0;
    json_doc_t* document = NULL;
    char* session_data = session_get(session_id);
    if (session_data == NULL) {
        ctx->response->send_data(ctx->response, "Session not found");
        return 0;
    }

    document = json_parse(session_data);
    if (document == NULL) {
        ctx->response->send_data(ctx->response, "");
        goto failed;
    }

    json_token_t* object = json_root(document);
    if (!json_is_object(object)) {
        ctx->response->send_data(ctx->response, "Session data is not object");
        goto failed;
    }
    json_token_t* token_user_id = json_object_get(object, "user_id");
    if (!json_is_number(token_user_id)) {
        ctx->response->send_data(ctx->response, "Session data is not valid");
        goto failed;
    }

    int ok = 0;
    const int user_id = json_int(token_user_id, &ok);
    if (!ok || user_id < 1) {
        ctx->response->send_data(ctx->response, "User is not valid");
        goto failed;
    }

    array_t* params = array_create();
    if (params == NULL) {
        ctx->response->send_data(ctx->response, "Error");
        goto failed;
    }

    mparams_fill_array(params,
        mparam_int(id, user_id)
    );
    user_t* user = user_get(params);
    array_free(params);

    if (user == NULL) {
        ctx->response->send_data(ctx->response, "User not found");
        goto failed;
    }

    httpctx_set_user(ctx, user);

    result = 1;

    failed:

    free(session_data);
    json_free(document);

    return result;
}
