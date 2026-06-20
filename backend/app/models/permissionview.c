#include <stdlib.h>
#include <stdio.h>

#include "db.h"
#include "permissionview.h"

static const char* __dbid = "postgresql";

enum permissionview_column {
    PERMISSIONVIEW_COL_ID = 0,
    PERMISSIONVIEW_COL_NAME,
    PERMISSIONVIEW_COLUMNS_COUNT
};

static const mcolumn_t __permissionview_columns[PERMISSIONVIEW_COLUMNS_COUNT] = {
    [PERMISSIONVIEW_COL_ID]   = { .name = "id",   .type = MODEL_INT, .is_primary = 1 },
    [PERMISSIONVIEW_COL_NAME] = { .name = "name", .type = MODEL_TEXT },
};

static const int __permissionview_primary_keys[] = { PERMISSIONVIEW_COL_ID };

static const mschema_t __permissionview_schema = {
    .table = "permission",
    .columns = __permissionview_columns,
    .columns_count = PERMISSIONVIEW_COLUMNS_COUNT,
    .primary_keys = __permissionview_primary_keys,
    .primary_keys_count = 1,
};

void* permissionview_instance(void) {
    permissionview_t* permission = calloc(1, sizeof * permission);
    if (permission == NULL) return NULL;

    if (!model_init(&permission->record, &__permissionview_schema)) {
        free(permission);
        return NULL;
    }

    return permission;
}

permissionview_t* permissionview_get(array_t* params) {
    return model_one(__dbid, permissionview_instance,
        "SELECT "
            "id, "
            "name "
        "FROM "
            "permission "
        "WHERE "
            "id = :id "
        "LIMIT 1"
        ,
        params
    );
}

array_t* permissionview_list(array_t* params) {
    return model_list(__dbid, permissionview_instance,
        "SELECT "
            "permission.id, "
            "permission.name "
        "FROM "
            "permission "

        "LEFT JOIN "
            "role_permission "
        "ON "
            "permission.id = role_permission.permission_id "

        "WHERE "
            "role_permission.role_id = :role_id "

        "ORDER BY "
            "permission.id ASC "
        ,
        params
    );
}

int permissionview_id(permissionview_t* permission) {
    return model_int(model_field(&permission->record, PERMISSIONVIEW_COL_ID));
}

const char* permissionview_name(permissionview_t* permission) {
    return str_get(model_text(model_field(&permission->record, PERMISSIONVIEW_COL_NAME)));
}
