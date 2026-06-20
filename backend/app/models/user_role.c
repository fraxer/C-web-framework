#include <stdlib.h>
#include <string.h>

#include "db.h"
#include "user_role.h"

static const char* __dbid = "postgresql";

enum user_role_column {
    USER_ROLE_COL_USER_ID = 0,
    USER_ROLE_COL_ROLE_ID,
    USER_ROLE_COLUMNS_COUNT
};

static const mcolumn_t __user_role_columns[USER_ROLE_COLUMNS_COUNT] = {
    [USER_ROLE_COL_USER_ID] = { .name = "user_id", .type = MODEL_INT, .is_primary = 1 },
    [USER_ROLE_COL_ROLE_ID] = { .name = "role_id", .type = MODEL_INT, .is_primary = 1 },
};

static const int __user_role_primary_keys[] = {
    USER_ROLE_COL_USER_ID,
    USER_ROLE_COL_ROLE_ID
};

static const mschema_t __user_role_schema = {
    .table = "user_role",
    .columns = __user_role_columns,
    .columns_count = USER_ROLE_COLUMNS_COUNT,
    .primary_keys = __user_role_primary_keys,
    .primary_keys_count = 2,
};

void* user_role_instance(void) {
    user_role_t* user_role = calloc(1, sizeof * user_role);
    if (user_role == NULL) return NULL;

    if (!model_init(&user_role->record, &__user_role_schema)) {
        free(user_role);
        return NULL;
    }

    return user_role;
}

user_role_t* user_role_get(array_t* params) {
    return model_one(__dbid, user_role_instance,
        "SELECT * FROM user_role WHERE user_id = :user_id AND role_id = :role_id LIMIT 1",
        params);
}

int user_role_create(user_role_t* user_role) {
    return model_create(__dbid, user_role);
}

int user_role_update(user_role_t* user_role) {
    return model_update(__dbid, user_role);
}

int user_role_delete(user_role_t* user_role) {
    return model_delete(__dbid, user_role);
}

void user_role_set_user_id(user_role_t* user_role, int user_id) {
    model_set_int(model_field(&user_role->record, USER_ROLE_COL_USER_ID), user_id);
}

void user_role_set_role_id(user_role_t* user_role, int role_id) {
    model_set_int(model_field(&user_role->record, USER_ROLE_COL_ROLE_ID), role_id);
}

int user_role_user_id(user_role_t* user_role) {
    return model_int(model_field(&user_role->record, USER_ROLE_COL_USER_ID));
}

int user_role_role_id(user_role_t* user_role) {
    return model_int(model_field(&user_role->record, USER_ROLE_COL_ROLE_ID));
}
