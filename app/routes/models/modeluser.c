#include "http1.h"
#include "user.h"
#include "userview.h"
#include "middleware.h"

void usercreate(httpctx_t* ctx) {
    user_t* user = user_instance();
    if (user == NULL) {
        ctx->response->status_code = 500;
        ctx->response->data(ctx->response, "error");
        model_free(user);
        return;
    }

    user_set_id(user, 13377);
    user_set_name(user, "alex");
    user_set_email(user, "pass");

    if (!user_create(user)) {
        ctx->response->status_code = 500;
        ctx->response->data(ctx->response, "error");
        model_free(user);
        return;
    }

    char* data = model_stringify(user);
    if (data == NULL) {
        ctx->response->status_code = 500;
        ctx->response->data(ctx->response, "error");
        model_free(user);
        return;
    }

    ctx->response->header_add(ctx->response, "Content-Type", "application/json");
    ctx->response->data(ctx->response, data);

    model_free(user);
    free(data);
}

void userget(httpctx_t* ctx) {
    const char* quser_id = ctx->request->query(ctx->request, "id");
    if (quser_id == NULL) {
        ctx->response->status_code = 400;
        ctx->response->data(ctx->response, "id not found in query");
        return;
    }

    const int userid = atoi(quser_id);


    mparams_create_array(params,
        mparam_int(id, userid)
    );
    user_t* user = user_get(params);
    array_free(params);

    if (user == NULL) {
        ctx->response->status_code = 500;
        ctx->response->data(ctx->response, "error");
        return;
    }

    char* data = model_stringify(user);
    if (data == NULL) {
        ctx->response->status_code = 500;
        ctx->response->data(ctx->response, "error");
        model_free(user);
        return;
    }
    ctx->response->header_add(ctx->response, "Content-Type", "application/json");
    ctx->response->data(ctx->response, data);

    free(data);
    model_free(user);
}

void userupdate(httpctx_t* ctx) {
    const int userid = 2;

    mparams_create_array(params,
        mparam_int(id, userid)
    );
    user_t* user = user_get(params);
    array_free(params);

    if (user == NULL) {
        ctx->response->status_code = 500;
        ctx->response->data(ctx->response, "error");
        return;
    }

    // user_set_id(user, 2);
    user_set_name(user, "Александр");
    user_set_email(user, "a@b.c");
    // user_set_enum(user, "V2");
    user_set_created_at(user, "2024-09-22 13:14:15");

    if (!user_update(user)) {
        ctx->response->status_code = 500;
        ctx->response->data(ctx->response, "error");
        model_free(user);
        return;
    }

    char* data = model_stringify(user);
    if (data == NULL) {
        ctx->response->status_code = 500;
        ctx->response->data(ctx->response, "error");
        model_free(user);
        return;
    }

    ctx->response->header_add(ctx->response, "Content-Type", "application/json");
    ctx->response->data(ctx->response, data);

    free(data);
    model_free(user);
}

void userdelete(httpctx_t* ctx) {
    const int userid = 5;

    mparams_create_array(params,
        mparam_int(id, userid)
    );
    user_t* user = user_get(params);
    array_free(params);

    if (user == NULL) {
        ctx->response->status_code = 500;
        ctx->response->data(ctx->response, "error");
        model_free(user);
        return;
    }

    if (!user_delete(user)) {
        ctx->response->status_code = 500;
        ctx->response->data(ctx->response, "error");
        model_free(user);
        return;
    }

    ctx->response->data(ctx->response, "user <> deleted");

    model_free(user);
}
