#include <stdlib.h>
#include <stdio.h>

#include "dbquery.h"
#include "dbresult.h"

const char* db() { return "postgresql"; }

int up(dbinstance_t* dbinst) {
    dbresult_t result = dbquery(dbinst,
        "CREATE TABLE public.user_role"
        "("
            "user_id    bigserial    NOT NULL,"
            "role_id    bigserial    NOT NULL,"

            "PRIMARY KEY (user_id, role_id)"
        ")"
    );

    if (!dbresult_ok(&result)) {
        printf("%s\n", dbresult_error_message(&result));
        dbresult_free(&result);
        return -1;
    }

    dbresult_free(&result);

    return 0;
}

int down(dbinstance_t* dbinst) {
    dbresult_t result = dbquery(dbinst, "DROP TABLE public.user_role");

    if (!dbresult_ok(&result)) {
        printf("%s\n", dbresult_error_message(&result));
        dbresult_free(&result);
        return -1;
    }

    dbresult_free(&result);

    return 0;
}
