#include <linux/limits.h>

#include "http1.h"
#include "log.h"
#include "json.h"
#include "db.h"
#include "view.h"
#include "model.h"
#include "helpers.h"
#include "auth.h"

void login(httpctx_t* ctx) {
    const char* email = ctx->request->query(ctx->request, "email");
    const char* password = ctx->request->query(ctx->request, "password");

    if (!validate_email(email)) {
        ctx->response->data(ctx->response, "Invalid email");
        return;
    }

    user_t* user = authenticate(email, password);
    if (user == NULL) {
        ctx->response->data(ctx->response, "Can't authenticate user");
        return;
    }

    ctx->response->cookie_add(ctx->response, (cookie_t){
        .name = "token",
        .value = user_token(user),
        .minutes = 60,
        .path = "/",
        .secure = 1,
        .http_only = 1,
        .same_site = "Lax"
    });

    ctx->response->data(ctx->response, model_stringify(user));

    model_free(user);
}

void registration(httpctx_t* ctx) {
    const char* email = ctx->request->query(ctx->request, "email");
    const char* password = ctx->request->query(ctx->request, "password");

    if (!validate_email(email)) {
        ctx->response->data(ctx->response, "Invalid email");
        return;
    }

    if (!validate_password(password)) {
        ctx->response->data(ctx->response, "Invalid password");
        return;
    }

    mparams_create_array(params,
        mparam_text(email, email)
    );
    user_t* user = user_get(params);
    array_free(params);

    if (user != NULL) {
        ctx->response->data(ctx->response, "User already exists");
        model_free(user);
        return;
    }

    user = user_instance();
    if (user == NULL) {
        ctx->response->data(ctx->response, "Can't create user");
        return;
    }

    str_t* secret = generate_secret(password);
    if (secret == NULL) {
        ctx->response->data(ctx->response, "Can't generate secret");
        model_free(user);
        return;
    }

    user_set_email(user, email);
    user_set_secret(user, str_get(secret));

    str_free(secret);

    if (!user_create(user)) {
        ctx->response->data(ctx->response, "Can't create user");
        model_free(user);
        return;
    }

    ctx->response->data(ctx->response, "done");

    model_free(user);
}
