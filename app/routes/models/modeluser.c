#include "http1.h"
#include "user.h"
#include "userview.h"
#include "middleware.h"

#define mparameters(...) (mfield_t*)&(mfield_t[NARG_(__VA_ARGS__,MW_NSEQ()) / 7]){__VA_ARGS__}, NARG_(__VA_ARGS__,MW_NSEQ()) / 7


int authenticate_by_name_pass(httpctx_t *ctx) {
    (void)ctx;
    // char* name = ctx->request->payloadf(ctx->request, "key1");
    // char* email = ctx->request->payloadf(ctx->request, "key1");

    // user_t* user = authenticate("mysql", name, email);
    // if (user == NULL) {
    //     free(name);
    //     free(email);
    //     ctx->response->data(ctx->response, "can't authenticate user");
    //     return 0;
    // }

    // // ctx->user->swap(ctx->user, user);

    // free(name);
    // free(email);

    return 1;
}

void usercreate(httpctx_t* ctx) {
    user_t* user = user_instance();
    if (user == NULL) {
        ctx->response->status_code = 500;
        ctx->response->data(ctx->response, "error");
        user_free(user);
        return;
    }

    // user_set_id(user, 13377);
    user_set_name(user, "alex");
    user_set_email(user, "pass");

    if (!user_create(user)) {
        ctx->response->status_code = 500;
        ctx->response->data(ctx->response, "error");
        user_free(user);
        return;
    }

    ctx->response->data(ctx->response, "done");

    user_free(user);
}

void userget(httpctx_t* ctx) {
    const char* quser_id = ctx->request->query(ctx->request, "id");
    if (quser_id == NULL) {
        ctx->response->status_code = 400;
        ctx->response->data(ctx->response, "id not found in query");
        return;
    }

    const int userid = atoi(quser_id);

    // mparam_t params[2] = {
    //     mparameter_int(id, userid),
    //     mparameter_string(name, "alexander")
    // };
    // user_t* user = user_get(params, 2);


    // user_t* user = user_get((mparam_t*)&(mparam_t[2]){
    //     mparameter_int(id, userid),
    //     mparameter_string(name, "alexander")
    // }, 2);


    user_t* user = user_get(mparameters(
        mparameter_int(id, userid)
    ));
    // user_t* user = user_get(mparams(
    //     mparam_int(id, userid),
    //     mparam_string(name, "alexander")
    // ));

    if (user == NULL) {
        ctx->response->status_code = 500;
        ctx->response->data(ctx->response, "error");
        user_free(user);
        return;
    }

    // ctx->response->data(ctx->response, model_text(&user.field.name));
    // or
    // ctx->response->data(ctx->response, user_name(user));
    // or
    char* data = user_stringify(user);
    if (data == NULL) {
        ctx->response->status_code = 500;
        ctx->response->data(ctx->response, "error");
        user_free(user);
        return;
    }
    ctx->response->header_add(ctx->response, "Content-Type", "application/json");
    ctx->response->data(ctx->response, data);

    free(data);
    user_free(user);
}

void userupdate(httpctx_t* ctx) {
    const int userid = 2;

    user_t* user = user_get(mparameters(
        mparameter_int(id, userid)
    ));
    if (user == NULL) {
        ctx->response->status_code = 500;
        ctx->response->data(ctx->response, "error");
        user_free(user);
        return;
    }

    user_set_name(user, "Василий 1");

    if (!user_update(user)) {
        ctx->response->status_code = 500;
        ctx->response->data(ctx->response, "error");
        user_free(user);
        return;
    }

    ctx->response->data(ctx->response, user_name(user));
    // ctx->response->data(ctx->response, user_stringify(user));

    user_free(user);
}

void userdelete(httpctx_t* ctx) {
    const int userid = 53;

    user_t* user = user_get(mparameters(
        mparameter_int(id, userid)
    ));
    if (user == NULL) {
        ctx->response->status_code = 500;
        ctx->response->data(ctx->response, "error");
        user_free(user);
        return;
    }

    if (!user_delete(user)) {
        ctx->response->status_code = 500;
        ctx->response->data(ctx->response, "error");
        user_free(user);
        return;
    }

    ctx->response->data(ctx->response, "user <> deleted");

    user_free(user);
}
