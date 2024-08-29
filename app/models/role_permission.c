#include <stdlib.h>
#include <string.h>

#include "db.h"
#include "role_permission.h"

static const char* __dbid = "postgresql";

static mfield_t* __first_field(void* arg);
static int __fields_count(void* arg);
static const char* __table(void* arg);
static const char** __unique_fields(void* arg);
static int __primary_key_count(void* arg);

role_permission_t* role_permission_instance(void) {
    role_permission_t* role_permission = malloc(sizeof * role_permission);
    if (role_permission == NULL)
        return NULL;

    role_permission_t st = {
        .base = {
            .fields_count = __fields_count,
            .primary_key_count = __primary_key_count,
            .first_field = __first_field,
            .table = __table,
            .primary_key = __unique_fields
        },
        .field = {
            mfield_int(role_id, 0),
            mfield_int(permission_id, 0),
        },
        .table = "role_permission",
        .primary_key = {
            "role_id",
            "permission_id",
        }
    };

    memcpy(role_permission, &st, sizeof st);

    return role_permission;
}

void role_permission_free(void* arg) {
    role_permission_t* role_permission = arg;
    if (role_permission == NULL)
        return;

    // model_free(&role_permission->field.id);
    // model_free(&role_permission->field.name);

    free(role_permission);
}

role_permission_t* role_permission_get(mfield_t* params, int params_count) {
    role_permission_t* role_permission = role_permission_instance();
    if (role_permission == NULL)
        return NULL;

    if (!model_get(__dbid, role_permission, params, params_count)) {
        role_permission_free(role_permission);
        return NULL;
    }

    return role_permission;
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
    model_set_int(&role_permission->field.role_id, role_id);
}

void role_permission_set_permission_id(role_permission_t* role_permission, int permission_id) {
    model_set_int(&role_permission->field.permission_id, permission_id);
}

int role_permission_role_id(role_permission_t* role_permission) {
    return model_int(&role_permission->field.role_id);
}

int role_permission_permission_id(role_permission_t* role_permission) {
    return model_int(&role_permission->field.permission_id);
}

mfield_t* __first_field(void* arg) {
    role_permission_t* role_permission = arg;
    if (role_permission == NULL)
        return NULL;

    return (void*)&role_permission->field;
}

int __fields_count(void* arg) {
    role_permission_t* role_permission = arg;
    if (role_permission == NULL)
        return 0;

    return sizeof(role_permission->field) / sizeof(role_permission->field.role_id);
}

const char* __table(void* arg) {
    role_permission_t* role_permission = arg;
    if (role_permission == NULL)
        return NULL;

    return role_permission->table;
}

const char** __unique_fields(void* arg) {
    role_permission_t* role_permission = arg;
    if (role_permission == NULL)
        return NULL;

    return (const char**)&role_permission->primary_key[0];
}

int __primary_key_count(void* arg) {
    role_permission_t* role_permission = arg;
    if (role_permission == NULL)
        return 0;

    return sizeof(role_permission->primary_key) / sizeof(role_permission->primary_key[0]);
}
