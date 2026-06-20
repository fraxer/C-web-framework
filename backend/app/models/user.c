#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>

#include "db.h"
#include "user.h"
#include "str.h"

static const char* __dbid = "postgresql.p1";

enum user_column {
    USER_COL_ID = 0,
    USER_COL_EMAIL,
    USER_COL_NAME,
    USER_COL_CREATED_AT,
    USER_COL_SECRET,
    USER_COLUMNS_COUNT
};

static const mcolumn_t __user_columns[USER_COLUMNS_COUNT] = {
    [USER_COL_ID]         = { .name = "id",         .type = MODEL_INT, .is_primary = 1, .auto_increment = 1 },
    [USER_COL_EMAIL]      = { .name = "email",      .type = MODEL_TEXT },
    [USER_COL_NAME]       = { .name = "name",       .type = MODEL_TEXT },
    [USER_COL_CREATED_AT] = { .name = "created_at", .type = MODEL_TIMESTAMP },
    [USER_COL_SECRET]     = { .name = "secret",     .type = MODEL_TEXT },
};

static const int __user_primary_keys[] = { USER_COL_ID };

static const mschema_t __user_schema = {
    .table = "\"user\"",
    .columns = __user_columns,
    .columns_count = USER_COLUMNS_COUNT,
    .primary_keys = __user_primary_keys,
    .primary_keys_count = 1,
};

void* user_instance(void) {
    user_t* user = calloc(1, sizeof * user);
    if (user == NULL) return NULL;

    if (!model_init(&user->record, &__user_schema)) {
        free(user);
        return NULL;
    }

    return user;
}

user_t* user_get(array_t* params) {
    str_t* sql = str_create_empty(256);
    if (sql == NULL) return NULL;

    str_append(sql, "SELECT * FROM ", 15);
    str_append(sql, __user_schema.table, strlen(__user_schema.table));

    if (params != NULL && array_size(params) > 0) {
        str_append(sql, " WHERE ", 7);
        for (size_t i = 0, n = 0; i < array_size(params); i++) {
            const mfield_t* f = (const mfield_t*)array_get(params, i);
            if (f == NULL || f->name == NULL) continue;
            if (n > 0) str_append(sql, " AND ", 5);
            str_append(sql, f->name, strlen(f->name));
            str_append(sql, " = :", 4);
            str_append(sql, f->name, strlen(f->name));
            n++;
        }
    }

    user_t* user = model_one(__dbid, user_instance, str_get(sql), params);
    str_free(sql);
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

void user_free(user_t* user) {
    if (user == NULL) return;

    // Wipe sensitive data before model_free
    explicit_bzero(user->salt, sizeof(user->salt));
    explicit_bzero(user->hash, sizeof(user->hash));

    model_free(user);
}

user_t* user_create_anonymous(void) {
    return NULL;
}

void user_set_id(user_t* user, int id) {
    model_set_int(model_field(&user->record, USER_COL_ID), id);
}

void user_set_name(user_t* user, const char* name) {
    model_set_text(model_field(&user->record, USER_COL_NAME), name);
}

void user_set_secret(user_t* user, const char* secret) {
    model_set_text(model_field(&user->record, USER_COL_SECRET), secret);
}

void user_set_email(user_t* user, const char* email) {
    model_set_text(model_field(&user->record, USER_COL_EMAIL), email);
}

void user_set_created_at(user_t* user, const char* value) {
    model_set_timestamp_from_str(model_field(&user->record, USER_COL_CREATED_AT), value);
}

int user_id(user_t* user) {
    return model_int(model_field(&user->record, USER_COL_ID));
}

const char* user_name(user_t* user) {
    return str_get(model_text(model_field(&user->record, USER_COL_NAME)));
}

const char* user_email(user_t* user) {
    return str_get(model_text(model_field(&user->record, USER_COL_EMAIL)));
}

const char* user_secret(user_t* user) {
    return str_get(model_text(model_field(&user->record, USER_COL_SECRET)));
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
