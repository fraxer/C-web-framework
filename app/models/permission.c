#include <stdlib.h>
#include <string.h>

#include "db.h"
#include "permission.h"

static const char* __dbid = "postgresql";

static mfield_t* __first_field(void* arg);
static int __fields_count(void* arg);
static const char* __table(void* arg);
static const char** __unique_fields(void* arg);
static int __primary_key_count(void* arg);

void* permission_instance(void) {
    permission_t* permission = malloc(sizeof * permission);
    if (permission == NULL)
        return NULL;

    permission_t st = {
        .base = {
            .fields_count = __fields_count,
            .primary_key_count = __primary_key_count,
            .first_field = __first_field,
            .table = __table,
            .primary_key = __unique_fields
        },
        .field = {
            mfield_int(id, 0),
            mfield_text(name, NULL)
        },
        .table = "permission",
        .primary_key = {
            "id",
        }
    };

    memcpy(permission, &st, sizeof st);

    return permission;
}

permission_t* permission_get(array_t* params) {
    return model_get(__dbid, permission_instance, params);
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
    model_set_int(&permission->field.id, id);
}

void permission_set_name(permission_t* permission, const char* name) {
    model_set_text(&permission->field.name, name);
}

int permission_id(permission_t* permission) {
    return model_int(&permission->field.id);
}

const char* permission_name(permission_t* permission) {
    return str_get(model_text(&permission->field.name));
}

mfield_t* __first_field(void* arg) {
    permission_t* permission = arg;
    if (permission == NULL)
        return NULL;

    return (void*)&permission->field;
}

int __fields_count(void* arg) {
    permission_t* permission = arg;
    if (permission == NULL)
        return 0;

    return sizeof(permission->field) / sizeof(mfield_t);
}

const char* __table(void* arg) {
    permission_t* permission = arg;
    if (permission == NULL)
        return NULL;

    return permission->table;
}

const char** __unique_fields(void* arg) {
    permission_t* permission = arg;
    if (permission == NULL)
        return NULL;

    return (const char**)&permission->primary_key[0];
}

int __primary_key_count(void* arg) {
    permission_t* permission = arg;
    if (permission == NULL)
        return 0;

    return sizeof(permission->primary_key) / sizeof(permission->primary_key[0]);
}
