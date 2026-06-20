#include <stdlib.h>
#include <stdio.h>

#include "db.h"
#include "userview.h"

enum userview_column {
    USERVIEW_COL_ID = 0,
    USERVIEW_COL_NAME,
    USERVIEW_COL_EMAIL,
    USERVIEW_COLUMNS_COUNT
};

static const mcolumn_t __userview_columns[USERVIEW_COLUMNS_COUNT] = {
    [USERVIEW_COL_ID]    = { .name = "id",    .type = MODEL_INT, .is_primary = 1 },
    [USERVIEW_COL_NAME]  = { .name = "name",  .type = MODEL_TEXT },
    [USERVIEW_COL_EMAIL] = { .name = "email", .type = MODEL_TEXT },
};

static const int __userview_primary_keys[] = { USERVIEW_COL_ID };

static const mschema_t __userview_schema = {
    .table = "\"user\"",
    .columns = __userview_columns,
    .columns_count = USERVIEW_COLUMNS_COUNT,
    .primary_keys = __userview_primary_keys,
    .primary_keys_count = 1,
};

void* userview_instance(void) {
    userview_t* user = calloc(1, sizeof * user);
    if (user == NULL) return NULL;

    if (!model_init(&user->record, &__userview_schema)) {
        free(user);
        return NULL;
    }

    return user;
}

userview_t* userview_get(array_t* params) {
    return model_one(POSTGRESQL, userview_instance,
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
    return model_list(POSTGRESQL, userview_instance,
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
    const int result = dbexec(POSTGRESQL,
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

int userview_id(userview_t* user) {
    return model_int(model_field(&user->record, USERVIEW_COL_ID));
}

const char* userview_name(userview_t* user) {
    return str_get(model_text(model_field(&user->record, USERVIEW_COL_NAME)));
}

const char* userview_email(userview_t* user) {
    return str_get(model_text(model_field(&user->record, USERVIEW_COL_EMAIL)));
}
