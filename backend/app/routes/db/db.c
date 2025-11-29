#include <linux/limits.h>

#include "http.h"
#include "websockets.h"
#include "log.h"
#include "json.h"
#include "db.h"
#include "storage.h"
#include "view.h"
#include "model.h"
#include "auth.h"

typedef struct {
    modelview_t base;
    struct {
        mfield_t id;
        mfield_t name;
        mfield_t email;
    } field;
} uv_t;

mfield_t* __first_field(void* arg) {
    uv_t* user = arg;
    if (user == NULL) return NULL;

    return (void*)&user->field;
}

int __fields_count(void* arg) {
    uv_t* user = arg;
    if (user == NULL) return 0;

    return sizeof(user->field) / sizeof(mfield_t);
}

void* user_instance(void) {
    uv_t* user = malloc(sizeof * user);
    if (user == NULL)
        return NULL;

    uv_t st = {
        .base = {
            .fields_count = __fields_count,
            .first_field = __first_field,
        },
        .field = {
            mfield_int(id, 0),
            mfield_text(name, NULL),
            mfield_text(email, NULL),
        }
    };

    memcpy(user, &st, sizeof st);

    return user;
}

void prepared_query(httpctx_t* ctx) {
    int param_ok = 0;
    const int user_id = query_param_int(ctx->request, "id", &param_ok);
    if (!param_ok) {
        ctx->response->send_default(ctx->response, 400);
        return;
    }

    array_t* params = array_create();
    if (params == NULL) {
        ctx->response->status_code = 500;
        ctx->response->send_data(ctx->response, "error");
        return;
    }
    mparams_fill_array(params,
        mparam_int(id, user_id),
        mparam_text(email, "admin@admin.admin")
    )

    uv_t* user = model_prepared_one(POSTGRESQL, user_instance, "user_get", params);

    array_free(params);

    if (user == NULL) {
        ctx->response->status_code = 500;
        ctx->response->send_data(ctx->response, "user not found");
        return;
    }

    ctx->response->send_model(ctx->response, user, display_fields("id", "email", "name"));

    model_free(user);
}
