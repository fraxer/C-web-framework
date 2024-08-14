#include <stdlib.h>
#include <string.h>

#include "db.h"
#include "user.h"

static const char* __dbid = "postgresql";

static mfield_t* __first_field(void* arg);
static int __first_field_count(void* arg);
static const char* __table(void* arg);
static const char** __unique_fields(void* arg);
static int __unique_fields_count(void* arg);
static char* __stringify(void* arg);

user_t* user_instance(void) {
    user_t* user = malloc(sizeof * user);
    if (user == NULL)
        return NULL;

    user_t st = {
        .base = {
            .fields_count = __first_field_count,
            .primary_key_count = __unique_fields_count,
            .first_field = __first_field,
            .table = __table,
            .primary_key = __unique_fields,
            .stringify = __stringify
        },
        .field = {
            mfield_int(id, 0),
            mfield_string(email, NULL),
            mfield_string(name, NULL)
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

void user_free(void* arg) {
    user_t* user = arg;
    if (user == NULL)
        return;

    // model_free(&user->field.id);
    // model_free(&user->field.name);

    free(user);
}

user_t* user_get(mfield_t* params, int params_count) {
    user_t* user = user_instance();
    if (user == NULL)
        return NULL;

    if (!model_get(__dbid, user, params, params_count)) {
        user_free(user);
        return NULL;
    }

    return user;
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
    model_set_string(&user->field.name, name);
}

void user_set_email(user_t* user, const char* email) {
    model_set_string(&user->field.email, email);
}

int user_id(user_t* user) {
    return model_int(&user->field.id);
}

const char* user_name(user_t* user) {
    return model_string(&user->field.name);
}

char* user_stringify(user_t* user) {
    if (user == NULL)
        return NULL;

    jsondoc_t* doc = json_init();
    if (!doc)
        return NULL;

    jsontok_t* object = json_create_object(doc);
    char* data = NULL;
    mfield_t* first_field = user->base.first_field(user);
    for (int i = 0; i < user->base.fields_count(user); i++) {
        mfield_t* field = first_field + i;

        switch (field->type) {
        case MODEL_INT:
            json_object_set(object, field->name, json_create_int(doc, model_int(field)));
            break;
        case MODEL_STRING:
            json_object_set(object, field->name, json_create_string(doc, model_string(field)));
            break;
        default:
            goto failed;
            break;
        }
    }

    data = json_stringify_detach(doc);

    failed:

    json_free(doc);

    return data;
}

mfield_t* __first_field(void* arg) {
    user_t* user = arg;
    if (user == NULL)
        return NULL;

    return (void*)&user->field;
}

int __first_field_count(void* arg) {
    user_t* user = arg;
    if (user == NULL)
        return 0;

    return sizeof(user->field) / sizeof(user->field.id);
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

int __unique_fields_count(void* arg) {
    user_t* user = arg;
    if (user == NULL)
        return 0;

    return sizeof(user->primary_key) / sizeof(user->primary_key[0]);
}

char* __stringify(void* arg) {
    user_t* user = arg;
    if (user == NULL)
        return NULL;

    return NULL;
}
