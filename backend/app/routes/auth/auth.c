#include "http.h"
#include "auth.h"
#include "appconfig.h"
#include "httpmiddlewares.h"

void login(httpctx_t* ctx) {
    int ok = 0;
    const char* email = query_param_char(ctx->request, "email", &ok);
    if (!ok) {
        ctx->response->status_code = 400;
        ctx->response->send_data(ctx->response, "email invalid in query");
        return;
    }

    const char* password = query_param_char(ctx->request, "password", &ok);
    if (!ok) {
        ctx->response->status_code = 400;
        ctx->response->send_data(ctx->response, "password invalid in query");
        return;
    }

    if (!validate_email(email)) {
        ctx->response->send_data(ctx->response, "Invalid email");
        return;
    }

    user_t* user = authenticate(email, password);
    if (user == NULL) {
        ctx->response->send_data(ctx->response, "Can't authenticate user");
        return;
    }

    json_doc_t* doc = json_root_create_object();
    json_token_t* object = json_root(doc);
    json_object_set(object, "user_id", json_create_number(model_int(&user->field.id)));

    char* session_id = session_create(json_stringify(doc));
    json_free(doc);

    if (session_id == NULL) {
        ctx->response->send_data(ctx->response, "Can't create session");
        user_free(user);
        return;
    }

    ctx->response->add_cookie(ctx->response, (cookie_t){
        .name = "session_id",
        .value = session_id,
        .seconds = appconfig()->sessionconfig.lifetime,
        .path = "/",
        .secure = 1,
        .http_only = 1,
        .same_site = "Lax"
    });

    free(session_id);

    ctx->response->send_model(ctx->response, user, display_fields("id", "email", "name", "created_at"));

    user_free(user);
}

void registration(httpctx_t* ctx) {
    int ok = 0;
    const char* email = query_param_char(ctx->request, "email", &ok);
    if (!ok) {
        ctx->response->status_code = 400;
        ctx->response->send_data(ctx->response, "email invalid in query");
        return;
    }

    const char* password = query_param_char(ctx->request, "password", &ok);
    if (!ok) {
        ctx->response->status_code = 400;
        ctx->response->send_data(ctx->response, "password invalid in query");
        return;
    }

    if (!validate_email(email)) {
        ctx->response->send_data(ctx->response, "Invalid email");
        return;
    }

    if (!validate_password(password)) {
        ctx->response->send_data(ctx->response, "Invalid password");
        return;
    }

    array_t* params = array_create();
    if (params == NULL) {
        ctx->response->send_data(ctx->response, "Error");
        return;
    }
    mparams_fill_array(params,
        mparam_text(email, email)
    );
    user_t* user = user_get(params);
    array_free(params);

    if (user != NULL) {
        ctx->response->send_data(ctx->response, "User already exists");
        model_free(user);
        return;
    }

    user = user_instance();
    if (user == NULL) {
        ctx->response->send_data(ctx->response, "Can't create user");
        return;
    }

    str_t* secret = generate_secret(password);
    if (secret == NULL) {
        ctx->response->send_data(ctx->response, "Can't generate secret");
        user_free(user);
        return;
    }

    user_set_email(user, email);
    user_set_secret(user, str_get(secret));

    str_free(secret);

    if (!user_create(user)) {
        ctx->response->send_data(ctx->response, "Can't create user");
        user_free(user);
        return;
    }

    ctx->response->send_data(ctx->response, "done");

    user_free(user);
}

void secret_page(httpctx_t* ctx) {
    middleware(
        middleware_http_auth(ctx)
    )

    ctx->response->send_data(ctx->response, "done");
}
