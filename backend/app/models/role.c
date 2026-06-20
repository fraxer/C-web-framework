#include <stdlib.h>
#include <string.h>

#include "db.h"
#include "role.h"

static const char* __dbid = "postgresql";

enum role_column {
    ROLE_COL_ID = 0,
    ROLE_COL_NAME,
    ROLE_COLUMNS_COUNT
};

static const mcolumn_t __role_columns[ROLE_COLUMNS_COUNT] = {
    [ROLE_COL_ID]   = { .name = "id",   .type = MODEL_INT, .is_primary = 1, .auto_increment = 1 },
    [ROLE_COL_NAME] = { .name = "name", .type = MODEL_TEXT },
};

static const int __role_primary_keys[] = { ROLE_COL_ID };

static const mschema_t __role_schema = {
    .table = "role",
    .columns = __role_columns,
    .columns_count = ROLE_COLUMNS_COUNT,
    .primary_keys = __role_primary_keys,
    .primary_keys_count = 1,
};

void* role_instance(void) {
    role_t* role = calloc(1, sizeof * role);
    if (role == NULL) return NULL;

    if (!model_init(&role->record, &__role_schema)) {
        free(role);
        return NULL;
    }

    return role;
}

role_t* role_get(array_t* params) {
    return model_one(__dbid, role_instance,
        "SELECT * FROM role WHERE id = :id LIMIT 1", params);
}

int role_create(role_t* role) {
    return model_create(__dbid, role);
}

int role_update(role_t* role) {
    return model_update(__dbid, role);
}

int role_delete(role_t* role) {
    return model_delete(__dbid, role);
}

void role_set_id(role_t* role, int id) {
    model_set_int(model_field(&role->record, ROLE_COL_ID), id);
}

void role_set_name(role_t* role, const char* name) {
    model_set_text(model_field(&role->record, ROLE_COL_NAME), name);
}

int role_id(role_t* role) {
    return model_int(model_field(&role->record, ROLE_COL_ID));
}

const char* role_name(role_t* role) {
    return str_get(model_text(model_field(&role->record, ROLE_COL_NAME)));
}
