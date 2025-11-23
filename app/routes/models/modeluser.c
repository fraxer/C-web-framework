#include "http.h"
#include "user.h"
#include "userview.h"
#include "middleware.h"

void usercreate(httpctx_t* ctx) {
    user_t* user = user_instance();
    if (user == NULL) {
        ctx->response->status_code = 500;
        ctx->response->send_data(ctx->response, "error");
        user_free(user);
        return;
    }

    user_set_id(user, 13377);
    user_set_name(user, "user");
    user_set_email(user, "pass");

    if (!user_create(user)) {
        ctx->response->status_code = 500;
        ctx->response->send_data(ctx->response, "error");
        user_free(user);
        return;
    }

    char* data = model_stringify(user, NULL);
    if (data == NULL) {
        ctx->response->status_code = 500;
        ctx->response->send_data(ctx->response, "error");
        user_free(user);
        return;
    }

    ctx->response->add_header(ctx->response, "Content-Type", "application/json");
    ctx->response->send_data(ctx->response, data);

    user_free(user);
    free(data);
}

void userget(httpctx_t* ctx) {
    int ok = 0;
    const int userid = query_param_int(ctx->request, "id", &ok);
    if (!ok) {
        ctx->response->status_code = 400;
        ctx->response->send_data(ctx->response, "id not found in query");
        return;
    }

    array_t* params = array_create();
    if (params == NULL) {
        ctx->response->status_code = 500;
        ctx->response->send_data(ctx->response, "error");
        return;
    }
    mparams_fill_array(params,
        mparam_int(id, userid)
    );
    user_t* user = user_get(params);
    array_free(params);

    if (user == NULL) {
        ctx->response->status_code = 500;
        ctx->response->send_data(ctx->response, "error");
        return;
    }

    char* data = model_stringify(user, NULL);
    if (data == NULL) {
        ctx->response->status_code = 500;
        ctx->response->send_data(ctx->response, "error");
        user_free(user);
        return;
    }
    ctx->response->add_header(ctx->response, "Content-Type", "application/json");
    ctx->response->send_data(ctx->response, data);

    free(data);
    user_free(user);
}

void userupdate(httpctx_t* ctx) {
    const int userid = 2;

    array_t* params = array_create();
    if (params == NULL) {
        ctx->response->status_code = 500;
        ctx->response->send_data(ctx->response, "error");
        return;
    }

    mparams_fill_array(params,
        mparam_int(id, userid)
    );
    user_t* user = user_get(params);
    array_free(params);

    if (user == NULL) {
        ctx->response->status_code = 500;
        ctx->response->send_data(ctx->response, "error");
        return;
    }

    user_set_id(user, 2);
    user_set_name(user, "Alexander");
    user_set_email(user, "a@b.c");
    user_set_created_at(user, "2024-09-22 13:14:15");

    if (!user_update(user)) {
        ctx->response->status_code = 500;
        ctx->response->send_data(ctx->response, "error");
        user_free(user);
        return;
    }

    char* data = model_stringify(user, NULL);
    if (data == NULL) {
        ctx->response->status_code = 500;
        ctx->response->send_data(ctx->response, "error");
        user_free(user);
        return;
    }

    ctx->response->add_header(ctx->response, "Content-Type", "application/json");
    ctx->response->send_data(ctx->response, data);

    free(data);
    user_free(user);
}

void userdelete(httpctx_t* ctx) {
    const int userid = 5;

    array_t* params = array_create();
    if (params == NULL) {
        ctx->response->status_code = 500;
        ctx->response->send_data(ctx->response, "error");
        return;
    }

    mparams_fill_array(params,
        mparam_int(id, userid)
    );
    user_t* user = user_get(params);
    array_free(params);

    if (user == NULL) {
        ctx->response->status_code = 500;
        ctx->response->send_data(ctx->response, "error");
        model_free(user);
        return;
    }

    if (!user_delete(user)) {
        ctx->response->status_code = 500;
        ctx->response->send_data(ctx->response, "error");
        model_free(user);
        return;
    }

    ctx->response->send_data(ctx->response, "user <> deleted");

    model_free(user);
}
