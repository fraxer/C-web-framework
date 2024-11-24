#include "http1.h"
#include "auth.h"
#include "appconfig.h"
#include "httpmiddlewares.h"

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

    jsondoc_t* doc = json_init();
    jsontok_t* object = json_create_object(doc);
    json_object_set(object, "user_id", json_create_int(doc, model_int(&user->field.id)));

    char* session_id = session_create(json_stringify(doc));
    json_free(doc);

    if (session_id == NULL) {
        ctx->response->data(ctx->response, "Can't create session");
        model_free(user);
        return;
    }

    ctx->response->cookie_add(ctx->response, (cookie_t){
        .name = "session_id",
        .value = session_id,
        .seconds = appconfig()->sessionconfig.lifetime,
        .path = "/",
        .secure = 1,
        .http_only = 1,
        .same_site = "Lax"
    });

    free(session_id);

    ctx->response->model(ctx->response, user, display_fields("id", "email", "name", "created_at"));

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

void secret_page(httpctx_t* ctx) {
    middleware(
        middleware_http_auth(ctx)
    )

    ctx->response->data(ctx->response, "done");
}
