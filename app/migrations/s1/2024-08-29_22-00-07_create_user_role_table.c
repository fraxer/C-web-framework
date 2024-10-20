#include <stdlib.h>
#include <stdio.h>

#include "dbquery.h"
#include "dbresult.h"

int up(dbinstance_t* dbinst) {
    dbresult_t* result = dbqueryf(dbinst,
        "CREATE TABLE public.user_role"
        "("
            "user_id    bigserial    NOT NULL,"
            "role_id    bigserial    NOT NULL,"

            "PRIMARY KEY (user_id, role_id)"
        ")"
    );

    int res = dbresult_ok(result);

    dbresult_free(result);

    return res;
}

int down(dbinstance_t* dbinst) {
    dbresult_t* result = dbqueryf(dbinst, "DROP TABLE public.user_role");

    int res = dbresult_ok(result);

    dbresult_free(result);

    return res;
}
