#include <stdlib.h>
#include <string.h>

#include "db.h"
#include "role.h"

static const char* __dbid = "postgresql";

static mfield_t* __first_field(void* arg);
static int __fields_count(void* arg);
static const char* __table(void* arg);
static const char** __unique_fields(void* arg);
static int __primary_key_count(void* arg);

void* role_instance(void) {
    role_t* role = malloc(sizeof * role);
    if (role == NULL)
        return NULL;

    role_t st = {
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
        .table = "role",
        .primary_key = {
            "id",
        }
    };

    memcpy(role, &st, sizeof st);

    return role;
}

role_t* role_get(array_t* params) {
    return model_get(__dbid, role_instance, params);
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
    model_set_int(&role->field.id, id);
}

void role_set_name(role_t* role, const char* name) {
    model_set_text(&role->field.name, name);
}

int role_id(role_t* role) {
    return model_int(&role->field.id);
}

const char* role_name(role_t* role) {
    return str_get(model_text(&role->field.name));
}

mfield_t* __first_field(void* arg) {
    role_t* role = arg;
    if (role == NULL)
        return NULL;

    return (void*)&role->field;
}

int __fields_count(void* arg) {
    role_t* role = arg;
    if (role == NULL)
        return 0;

    return sizeof(role->field) / sizeof(mfield_t);
}

const char* __table(void* arg) {
    role_t* role = arg;
    if (role == NULL)
        return NULL;

    return role->table;
}

const char** __unique_fields(void* arg) {
    role_t* role = arg;
    if (role == NULL)
        return NULL;

    return (const char**)&role->primary_key[0];
}

int __primary_key_count(void* arg) {
    role_t* role = arg;
    if (role == NULL)
        return 0;

    return sizeof(role->primary_key) / sizeof(role->primary_key[0]);
}
