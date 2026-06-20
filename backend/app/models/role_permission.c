#include <stdlib.h>
#include <string.h>

#include "db.h"
#include "role_permission.h"

static const char* __dbid = "postgresql";

enum role_permission_column {
    ROLE_PERMISSION_COL_ROLE_ID = 0,
    ROLE_PERMISSION_COL_PERMISSION_ID,
    ROLE_PERMISSION_COLUMNS_COUNT
};

static const mcolumn_t __role_permission_columns[ROLE_PERMISSION_COLUMNS_COUNT] = {
    [ROLE_PERMISSION_COL_ROLE_ID]       = { .name = "role_id",       .type = MODEL_INT, .is_primary = 1 },
    [ROLE_PERMISSION_COL_PERMISSION_ID] = { .name = "permission_id", .type = MODEL_INT, .is_primary = 1 },
};

static const int __role_permission_primary_keys[] = {
    ROLE_PERMISSION_COL_ROLE_ID,
    ROLE_PERMISSION_COL_PERMISSION_ID
};

static const mschema_t __role_permission_schema = {
    .table = "role_permission",
    .columns = __role_permission_columns,
    .columns_count = ROLE_PERMISSION_COLUMNS_COUNT,
    .primary_keys = __role_permission_primary_keys,
    .primary_keys_count = 2,
};

void* role_permission_instance(void) {
    role_permission_t* role_permission = calloc(1, sizeof * role_permission);
    if (role_permission == NULL) return NULL;

    if (!model_init(&role_permission->record, &__role_permission_schema)) {
        free(role_permission);
        return NULL;
    }

    return role_permission;
}

role_permission_t* role_permission_get(array_t* params) {
    return model_one(__dbid, role_permission_instance,
        "SELECT * FROM role_permission WHERE role_id = :role_id AND permission_id = :permission_id LIMIT 1",
        params);
}

int role_permission_create(role_permission_t* role_permission) {
    return model_create(__dbid, role_permission);
}

int role_permission_update(role_permission_t* role_permission) {
    return model_update(__dbid, role_permission);
}

int role_permission_delete(role_permission_t* role_permission) {
    return model_delete(__dbid, role_permission);
}

void role_permission_set_role_id(role_permission_t* role_permission, int role_id) {
    model_set_int(model_field(&role_permission->record, ROLE_PERMISSION_COL_ROLE_ID), role_id);
}

void role_permission_set_permission_id(role_permission_t* role_permission, int permission_id) {
    model_set_int(model_field(&role_permission->record, ROLE_PERMISSION_COL_PERMISSION_ID), permission_id);
}

int role_permission_role_id(role_permission_t* role_permission) {
    return model_int(model_field(&role_permission->record, ROLE_PERMISSION_COL_ROLE_ID));
}

int role_permission_permission_id(role_permission_t* role_permission) {
    return model_int(model_field(&role_permission->record, ROLE_PERMISSION_COL_PERMISSION_ID));
}
