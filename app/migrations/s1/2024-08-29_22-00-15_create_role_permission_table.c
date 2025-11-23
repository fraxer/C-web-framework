#include <stdlib.h>
#include <stdio.h>

#include "dbquery.h"
#include "dbresult.h"

int up(const char* dbid) {
    dbresult_t* result = dbqueryf(dbid,
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
