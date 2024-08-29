#include <stdlib.h>
#include <stdio.h>

#include "dbquery.h"
#include "dbresult.h"

const char* db() { return "postgresql"; }

int up(dbinstance_t* dbinst) {
    dbresult_t result = dbquery(dbinst,
        "CREATE TABLE public.user"
        "("
            "id    bigserial    NOT NULL PRIMARY KEY,"
            "email varchar(100) NOT NULL DEFAULT '',"
            "name  varchar(100) NOT NULL DEFAULT ''"
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
    dbresult_t result = dbquery(dbinst, "DROP TABLE public.user");

    if (!dbresult_ok(&result)) {
        printf("%s\n", dbresult_error_message(&result));
        dbresult_free(&result);
        return -1;
    }

    dbresult_free(&result);

    return 0;
}