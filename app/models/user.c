#include <stdlib.h>
#include <string.h>

#include "db.h"
#include "user.h"

static const char* __dbid = "postgresql";

static mfield_t* __first_field(void* arg);
static int __fields_count(void* arg);
static const char* __table(void* arg);
static const char** __unique_fields(void* arg);
static int __primary_key_count(void* arg);

void* user_instance(void) {
    user_t* user = malloc(sizeof * user);
    if (user == NULL) return NULL;

    user_t st = {
        .base = {
            .fields_count = __fields_count,
            .primary_key_count = __primary_key_count,
            .first_field = __first_field,
            .table = __table,
            .primary_key = __unique_fields
        },
        .field = {
            mfield_int(id, 0),
            mfield_text(email, NULL),
            mfield_text(name, NULL),
            mfield_enum(enm, NULL, "V1", "V2", "V3"),
            mfield_timestamp(dt, NULL)
        },
        .table = "\"user\"",
        .primary_key = {
            "id",
            "name",
        }
    };

    memcpy(user, &st, sizeof st);

    return user;
}

user_t* user_get(array_t* params) {
    return model_get(__dbid, user_instance, params);
}

int user_create(user_t* user) {
    return model_create(__dbid, user);
}

int user_update(user_t* user) {
    return model_update(__dbid, user);
}

int user_delete(user_t* user) {
    return model_delete(__dbid, user);
}

user_t* user_create_anonymous(void) {
    return NULL;
}

void user_set_id(user_t* user, int id) {
    model_set_int(&user->field.id, id);
}

void user_set_name(user_t* user, const char* name) {
    model_set_text(&user->field.name, name);
}

void user_set_email(user_t* user, const char* email) {
    model_set_text(&user->field.email, email);
}

void user_set_enum(user_t* user, const char* value) {
    model_set_enum(&user->field.enm, value);
}

void user_set_ts(user_t* user, const char* value) {
    model_set_timestamp_from_str(&user->field.dt, value);
}

int user_id(user_t* user) {
    return model_int(&user->field.id);
}

const char* user_name(user_t* user) {
    return str_get(model_text(&user->field.name));
}

const char* user_email(user_t* user) {
    return str_get(model_text(&user->field.email));
}

mfield_t* __first_field(void* arg) {
    user_t* user = arg;
    if (user == NULL)
        return NULL;

    return (void*)&user->field;
}

int __fields_count(void* arg) {
    user_t* user = arg;
    if (user == NULL)
        return 0;

    return sizeof(user->field) / sizeof(mfield_t);
}

const char* __table(void* arg) {
    user_t* user = arg;
    if (user == NULL)
        return NULL;

    return user->table;
}

const char** __unique_fields(void* arg) {
    user_t* user = arg;
    if (user == NULL)
        return NULL;

    return (const char**)&user->primary_key[0];
}

int __primary_key_count(void* arg) {
    user_t* user = arg;
    if (user == NULL)
        return 0;

    return sizeof(user->primary_key) / sizeof(user->primary_key[0]);
}
