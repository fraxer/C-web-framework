#include <stdlib.h>
#include <string.h>

#include "db.h"
#include "permission.h"

static const char* __dbid = "postgresql";

enum permission_column {
    PERMISSION_COL_ID = 0,
    PERMISSION_COL_NAME,
    PERMISSION_COLUMNS_COUNT
};

static const mcolumn_t __permission_columns[PERMISSION_COLUMNS_COUNT] = {
    [PERMISSION_COL_ID]   = { .name = "id",   .type = MODEL_INT, .is_primary = 1, .auto_increment = 1 },
    [PERMISSION_COL_NAME] = { .name = "name", .type = MODEL_TEXT },
};

static const int __permission_primary_keys[] = { PERMISSION_COL_ID };

static const mschema_t __permission_schema = {
    .table = "permission",
    .columns = __permission_columns,
    .columns_count = PERMISSION_COLUMNS_COUNT,
    .primary_keys = __permission_primary_keys,
    .primary_keys_count = 1,
};

void* permission_instance(void) {
    permission_t* permission = calloc(1, sizeof * permission);
    if (permission == NULL) return NULL;

    if (!model_init(&permission->record, &__permission_schema)) {
        free(permission);
        return NULL;
    }

    return permission;
}

permission_t* permission_get(array_t* params) {
    return model_one(__dbid, permission_instance,
        "SELECT * FROM permission WHERE id = :id LIMIT 1", params);
}

int permission_create(permission_t* permission) {
    return model_create(__dbid, permission);
}

int permission_update(permission_t* permission) {
    return model_update(__dbid, permission);
}

int permission_delete(permission_t* permission) {
    return model_delete(__dbid, permission);
}

void permission_set_id(permission_t* permission, int id) {
    model_set_int(model_field(&permission->record, PERMISSION_COL_ID), id);
}

void permission_set_name(permission_t* permission, const char* name) {
    model_set_text(model_field(&permission->record, PERMISSION_COL_NAME), name);
}

int permission_id(permission_t* permission) {
    return model_int(model_field(&permission->record, PERMISSION_COL_ID));
}

const char* permission_name(permission_t* permission) {
    return str_get(model_text(model_field(&permission->record, PERMISSION_COL_NAME)));
}
