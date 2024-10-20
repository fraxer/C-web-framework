#include <stdlib.h>
#include <stdio.h>

#include "db.h"
#include "userview.h"

static const char* __dbid = "postgresql";

static mfield_t* __first_field(void* arg);
static int __fields_count(void* arg);

void* userview_instance(void) {
    userview_t* user = malloc(sizeof * user);
    if (user == NULL)
        return NULL;

    userview_t st = {
        .base = {
            .fields_count = __fields_count,
            .first_field = __first_field,
        },
        .field = {
            mfield_int(id, 0),
            mfield_text(name, NULL),
            mfield_text(email, NULL),
        }
    };

    memcpy(user, &st, sizeof st);

    return user;
}

userview_t* userview_get(array_t* params) {
    return model_one(__dbid, userview_instance, 
        "SELECT "
            "id, "
            "name, "
            "email "
        "FROM "
            "\"user\" "
        "WHERE "
            "id = :id "
        "LIMIT 1",
        params
    );
}

array_t* userview_list() {
    return model_list(__dbid, userview_instance,
        "SELECT "
            "id, "
            "name, "
            "email "
        "FROM "
            "\"user\" "
        "ORDER BY "
            "id ASC "
        "LIMIT 1000 OFFSET 0", NULL
    );
}

int userview_execute(array_t* params) {
    const int result = model_execute(__dbid,
        "UPDATE "
            "\"user\" "
        "SET"
            "id = :id "
        "WHERE "
            "name = 'test' "
        ,
        params
    );

    return result;
}

mfield_t* __first_field(void* arg) {
    userview_t* user = arg;
    if (user == NULL) return NULL;

    return (void*)&user->field;
}

int __fields_count(void* arg) {
    userview_t* user = arg;
    if (user == NULL) return 0;

    return sizeof(user->field) / sizeof(mfield_t);
}