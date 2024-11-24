#include "http1.h"
#include "log.h"
#include "model.h"
#include "auth.h"
#include "appconfig.h"

void session(httpctx_t* ctx) {
    const int user_id = 12;

    jsondoc_t* doc = json_init();
    jsontok_t* object = json_create_object(doc);
    json_object_set(object, "user_id", json_create_int(doc, user_id));

    char* session_id = session_create(json_stringify(doc));
    json_free(doc);

    if (session_id == NULL) {
        ctx->response->data(ctx->response, "Can't create session");
        return;
    }

    char* session_data = session_get(session_id);
    if (session_data == NULL) {
        free(session_id);
        ctx->response->data(ctx->response, "Can't get session data");
        return;
    }

    free(session_data);

    ctx->response->cookie_add(ctx->response, (cookie_t){
        .name = "session_id",
        .value = session_id,
        .seconds = appconfig()->sessionconfig.lifetime,
        .path = "/",
        .secure = 1,
        .http_only = 1,
        .same_site = "Lax"
    });

    if (!session_update(session_id, "data")) {
        log_error("Can't update session");
    }

    if (!session_destroy(session_id)) {
        log_error("Can't destroy session");
    }

    free(session_id);

    ctx->response->data(ctx->response, "done");
}