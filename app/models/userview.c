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

userview_t* userview_get(userview_get_params_t* params) {
    return model_one(__dbid, userview_instance,
        "SELECT "
            "id, "
            "name, "
            "email "
        "FROM "
            "\"user\" "
        "WHERE "
            "id = %d "
        "LIMIT 1"
        ,
        model_int(&params->id)
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
        "LIMIT 1000 OFFSET 0"
    );
}

int userview_execute(userview_execute_params_t* params) {
    return model_execute(__dbid,
        "UPDATE "
            "\"user\" "
        "SET"
            "id = %d "
        "WHERE "
            "name = 'test' "
        ,
        model_int(&params->id)
    );
}

mfield_t* __first_field(void* arg) {
    userview_t* user = arg;
    if (user == NULL) return NULL;

    return (void*)&user->field;
}

int __fields_count(void* arg) {
    userview_t* user = arg;
    if (user == NULL) return 0;

    return sizeof(user->field) / sizeof(user->field.id);
}