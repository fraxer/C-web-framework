#include <stdlib.h>
#include <stdio.h>

#include "db.h"
#include "permissionview.h"

static const char* __dbid = "postgresql";

static mfield_t* __first_field(void* arg);
static int __fields_count(void* arg);

void* permissionview_instance(void) {
    permissionview_t* permission = malloc(sizeof * permission);
    if (permission == NULL)
        return NULL;

    permissionview_t st = {
        .base = {
            .fields_count = __fields_count,
            .first_field = __first_field,
        },
        .field = {
            mfield_int(id, 0),
            mfield_text(name, NULL),
        }
    };

    memcpy(permission, &st, sizeof st);

    return permission;
}

permissionview_t* permissionview_get(permissionview_get_params_t* params) {
    return model_one(__dbid, permissionview_instance,
        "SELECT "
            "id, "
            "name "
        "FROM "
            "permission "
        "WHERE "
            "id = %d "
        "LIMIT 1"
        ,
        model_int(&params->id)
    );
}

array_t* permissionview_list(permissionview_list_params_t* params) {
    return model_list(__dbid, permissionview_instance,
        "SELECT "
            "permission.id, "
            "permission.name "
        "FROM "
            "permission "

        "LEFT JOIN "
            "role_permission "
        "ON "
            "permission.id = role_permission.permission_id "

        "WHERE "
            "role_permission.role_id = %d "

        "ORDER BY "
            "permission.id ASC "
        ,
        model_int(&params->role_id)
    );
}

mfield_t* __first_field(void* arg) {
    permissionview_t* permission = arg;
    if (permission == NULL) return NULL;

    return (void*)&permission->field;
}

int __fields_count(void* arg) {
    permissionview_t* permission = arg;
    if (permission == NULL) return 0;

    return sizeof(permission->field) / sizeof(mfield_t);
}