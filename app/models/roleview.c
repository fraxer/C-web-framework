#include <stdlib.h>
#include <stdio.h>

#include "db.h"
#include "roleview.h"

static const char* __dbid = "postgresql";

static mfield_t* __first_field(void* arg);
static int __fields_count(void* arg);

void* roleview_instance(void) {
    roleview_t* role = malloc(sizeof * role);
    if (role == NULL)
        return NULL;

    roleview_t st = {
        .base = {
            .fields_count = __fields_count,
            .first_field = __first_field,
        },
        .field = {
            mfield_int(id, 0),
            mfield_string(name, NULL),
        }
    };

    memcpy(role, &st, sizeof st);

    return role;
}

roleview_t* roleview_get(roleview_get_params_t* params) {
    return modelview_one(__dbid, roleview_instance,
        "SELECT "
            "id, "
            "name "
        "FROM "
            "role "
        "WHERE "
            "id = %d "
        "LIMIT 1"
        ,
        model_int(&params->id)
    );
}

array_t* roleview_list(roleview_list_params_t* params) {
    return modelview_list(__dbid, roleview_instance,
        "SELECT "
            "role.id, "
            "role.name "
        "FROM "
            "role "

        "LEFT JOIN "
            "user_role "
        "ON "
            "role.id = user_role.role_id "

        "WHERE "
            "user_role.user_id = %d "

        "ORDER BY "
            "role.id ASC "
        ,
        model_int(&params->user_id)
    );
}

mfield_t* __first_field(void* arg) {
    roleview_t* role = arg;
    if (role == NULL) return NULL;

    return (void*)&role->field;
}

int __fields_count(void* arg) {
    roleview_t* role = arg;
    if (role == NULL) return 0;

    return sizeof(role->field) / sizeof(role->field.id);
}