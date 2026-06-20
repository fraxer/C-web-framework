#include <stdlib.h>
#include <stdio.h>

#include "db.h"
#include "roleview.h"

static const char* __dbid = "postgresql";

enum roleview_column {
    ROLEVIEW_COL_ID = 0,
    ROLEVIEW_COL_NAME,
    ROLEVIEW_COLUMNS_COUNT
};

static const mcolumn_t __roleview_columns[ROLEVIEW_COLUMNS_COUNT] = {
    [ROLEVIEW_COL_ID]   = { .name = "id",   .type = MODEL_INT, .is_primary = 1 },
    [ROLEVIEW_COL_NAME] = { .name = "name", .type = MODEL_TEXT },
};

static const int __roleview_primary_keys[] = { ROLEVIEW_COL_ID };

static const mschema_t __roleview_schema = {
    .table = "role",
    .columns = __roleview_columns,
    .columns_count = ROLEVIEW_COLUMNS_COUNT,
    .primary_keys = __roleview_primary_keys,
    .primary_keys_count = 1,
};

void* roleview_instance(void) {
    roleview_t* role = calloc(1, sizeof * role);
    if (role == NULL) return NULL;

    if (!model_init(&role->record, &__roleview_schema)) {
        free(role);
        return NULL;
    }

    return role;
}

roleview_t* roleview_get(array_t* params) {
    return model_one(__dbid, roleview_instance,
        "SELECT "
            "id, "
            "name "
        "FROM "
            "role "
        "WHERE "
            "id = :id "
        "LIMIT 1"
        ,
        params
    );
}

array_t* roleview_list(array_t* params) {
    return model_list(__dbid, roleview_instance,
        "SELECT "
            "role.id, "
            "role.name "
        "FROM "
            "role "

        "LEFT JOIN "
            "user_role "
        "ON "
            "role.id = user_role.role_id "

        "WHERE "
            "user_role.user_id = :user_id "

        "ORDER BY "
            "role.id ASC "
        ,
        params
    );
}

int roleview_id(roleview_t* role) {
    return model_int(model_field(&role->record, ROLEVIEW_COL_ID));
}

const char* roleview_name(roleview_t* role) {
    return str_get(model_text(model_field(&role->record, ROLEVIEW_COL_NAME)));
}
