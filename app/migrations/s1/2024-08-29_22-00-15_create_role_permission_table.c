#include <stdlib.h>
#include <stdio.h>

#include "dbquery.h"
#include "dbresult.h"

int up(dbinstance_t* dbinst) {
    dbresult_t* result = dbqueryf(dbinst,
        "CREATE TABLE public.role_permission"
        "("
            "role_id          bigserial    NOT NULL,"
            "permission_id    bigserial    NOT NULL,"

            "PRIMARY KEY (role_id, permission_id)"
        ")"
    );

    int res = dbresult_ok(result);

    dbresult_free(result);

    return res;
}

int down(dbinstance_t* dbinst) {
    dbresult_t* result = dbqueryf(dbinst, "DROP TABLE public.role_permission");

    int res = dbresult_ok(result);

    dbresult_free(result);

    return res;
}
