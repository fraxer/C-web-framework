#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>

#include "db.h"
#include "user.h"

static const char* __dbid = "postgresql.p1";

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
            // mfield_enum(enm, NULL, "V1", "V2", "V3"),
            mfield_timestamp(created_at, (tm_t){0}),
            mfield_text(secret, NULL),
        },
        .table = "\"user\"",
        .primary_key = {
            "id",
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

void user_free(user_t* user) {
    if (user == NULL) return;

    // Wipe sensitive data before model_free
    explicit_bzero(user->salt, sizeof(user->salt));
    explicit_bzero(user->hash, sizeof(user->hash));
    explicit_bzero(user->table, sizeof(user->table));

    model_free(user);
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

void user_set_secret(user_t* user, const char* secret) {
    model_set_text(&user->field.secret, secret);
}

void user_set_email(user_t* user, const char* email) {
    model_set_text(&user->field.email, email);
}

void user_set_created_at(user_t* user, const char* value) {
    model_set_timestamp_from_str(&user->field.created_at, value);
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

const char* user_secret(user_t* user) {
    return str_get(model_text(&user->field.secret));
}

const char* user_salt(user_t* user) {
    const char* secret = user_secret(user);
    if (secret == NULL) return NULL;

    const char* at_pos = strchr(secret, '$');
    if (!at_pos) return NULL;

    const char* salt = at_pos + 1;

    at_pos = strchr(at_pos + 1, '$');
    if (!at_pos) at_pos = secret + strlen(secret);

    const size_t length = at_pos - salt;

    strncpy(user->salt, salt, length);
    user->salt[length] = 0;

    return user->salt;
}

const char* user_hash(user_t* user) {
    const char* secret = user_secret(user);
    if (secret == NULL) return NULL;

    const char* at_pos = strrchr(secret, '$');
    if (!at_pos) return NULL;

    const char* hash = at_pos + 1;

    const size_t length = (secret + strlen(secret)) - hash;

    strncpy(user->hash, hash, length);
    user->hash[length] = 0;

    return user->hash;
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
