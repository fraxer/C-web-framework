#include "http.h"
#include "log.h"
#include "model.h"
#include "auth.h"
#include "appconfig.h"

void session(httpctx_t* ctx) {
    const int user_id = 12;

    json_doc_t* doc = json_root_create_object();
    json_token_t* object = json_root(doc);
    json_object_set(object, "user_id", json_create_number(user_id));

    char* session_id = session_create("backend", json_stringify(doc), 300);
    json_free(doc);

    if (session_id == NULL) {
        ctx->response->send_data(ctx->response, "Can't create session");
        return;
    }

    char* session_data = session_get("backend", session_id);
    if (session_data == NULL) {
        free(session_id);
        ctx->response->send_data(ctx->response, "Can't get session data");
        return;
    }

    free(session_data);

    ctx->response->add_cookie(ctx->response, (cookie_t){
        .name = "session_id",
        .value = session_id,
        .seconds = 300,
        .path = "/",
        .secure = 1,
        .http_only = 1,
        .same_site = "Lax"
    });

    if (!session_update("backend", session_id, "data")) {
        log_error("Can't update session");
    }

    if (!session_destroy("backend", session_id)) {
        log_error("Can't destroy session");
    }

    free(session_id);

    ctx->response->send_data(ctx->response, "done");
}