#include <stdlib.h>
#include <string.h>

#include "db.h"
#include "user_role.h"

static const char* __dbid = "postgresql";

static mfield_t* __first_field(void* arg);
static int __fields_count(void* arg);
static const char* __table(void* arg);
static const char** __unique_fields(void* arg);
static int __primary_key_count(void* arg);

user_role_t* user_role_instance(void) {
    user_role_t* user_role = malloc(sizeof * user_role);
    if (user_role == NULL)
        return NULL;

    user_role_t st = {
        .base = {
            .fields_count = __fields_count,
            .primary_key_count = __primary_key_count,
            .first_field = __first_field,
            .table = __table,
            .primary_key = __unique_fields
        },
        .field = {
            mfield_int(user_id, 0),
            mfield_int(role_id, 0),
        },
        .table = "user_role",
        .primary_key = {
            "user_id",
            "role_id",
        }
    };

    memcpy(user_role, &st, sizeof st);

    return user_role;
}

user_role_t* user_role_get(mfield_t* params, int params_count) {
    user_role_t* user_role = user_role_instance();
    if (user_role == NULL)
        return NULL;

    if (!model_get(__dbid, user_role, params, params_count)) {
        model_free(user_role);
        return NULL;
    }

    return user_role;
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
    model_set_int(&user_role->field.user_id, user_id);
}

void user_role_set_role_id(user_role_t* user_role, int role_id) {
    model_set_int(&user_role->field.role_id, role_id);
}

int user_role_user_id(user_role_t* user_role) {
    return model_int(&user_role->field.user_id);
}

int user_role_role_id(user_role_t* user_role) {
    return model_int(&user_role->field.role_id);
}

mfield_t* __first_field(void* arg) {
    user_role_t* user_role = arg;
    if (user_role == NULL)
        return NULL;

    return (void*)&user_role->field;
}

int __fields_count(void* arg) {
    user_role_t* user_role = arg;
    if (user_role == NULL)
        return 0;

    return sizeof(user_role->field) / sizeof(mfield_t);
}

const char* __table(void* arg) {
    user_role_t* user_role = arg;
    if (user_role == NULL)
        return NULL;

    return user_role->table;
}

const char** __unique_fields(void* arg) {
    user_role_t* user_role = arg;
    if (user_role == NULL)
        return NULL;

    return (const char**)&user_role->primary_key[0];
}

int __primary_key_count(void* arg) {
    user_role_t* user_role = arg;
    if (user_role == NULL)
        return 0;

    return sizeof(user_role->primary_key) / sizeof(user_role->primary_key[0]);
}
