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

enum uv_column {
    UV_COL_ID = 0,
    UV_COL_NAME,
    UV_COL_EMAIL,
    UV_COLUMNS_COUNT
};

static const mcolumn_t __uv_columns[UV_COLUMNS_COUNT] = {
    [UV_COL_ID]    = { .name = "id",    .type = MODEL_INT, .is_primary = 1 },
    [UV_COL_NAME]  = { .name = "name",  .type = MODEL_TEXT },
    [UV_COL_EMAIL] = { .name = "email", .type = MODEL_TEXT },
};

static const int __uv_primary_keys[] = { UV_COL_ID };

static const mschema_t __uv_schema = {
    .table = "\"user\"",
    .columns = __uv_columns,
    .columns_count = UV_COLUMNS_COUNT,
    .primary_keys = __uv_primary_keys,
    .primary_keys_count = 1,
};

typedef struct {
    model_t record;
} uv_t;

void* user_instance(void) {
    uv_t* user = calloc(1, sizeof * user);
    if (user == NULL)
        return NULL;

    if (!model_init(&user->record, &__uv_schema)) {
        free(user);
        return NULL;
    }

    return user;
}

void prepared_query(httpctx_t* ctx) {
    int param_ok = 0;
    const int user_id = query_param_int(ctx->request->query_, "id", &param_ok);
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

    uv_t* user = model_prepared_one(POSTGRESQL, user_instance, "user_get",
        "SELECT id, name, email FROM \"user\" WHERE id = :id AND email = :email LIMIT 1",
        params);

    array_free(params);

    if (user == NULL) {
        ctx->response->status_code = 500;
        ctx->response->send_data(ctx->response, "user not found");
        return;
    }

    ctx->response->send_model(ctx->response, user, display_fields("id", "email", "name"));

    model_free(user);
}
