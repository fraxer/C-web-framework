#include <stdlib.h>
#include <stdio.h>

#include "dbquery.h"
#include "dbresult.h"

int up(dbinstance_t* dbinst) {
    dbresult_t* result = dbqueryf(dbinst,
        "CREATE TABLE public.permission"
        "("
            "id    bigserial    NOT NULL PRIMARY KEY,"
            "name  varchar(100) NOT NULL DEFAULT ''"
        ")"
    );

    int res = dbresult_ok(result);

    dbresult_free(result);

    return res;
}

int down(dbinstance_t* dbinst) {
    dbresult_t* result = dbqueryf(dbinst, "DROP TABLE public.permission");

    int res = dbresult_ok(result);

    dbresult_free(result);

    return res;
}
