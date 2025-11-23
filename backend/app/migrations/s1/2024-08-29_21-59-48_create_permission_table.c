#include <stdlib.h>
#include <stdio.h>

#include "dbquery.h"
#include "dbresult.h"

int up(const char* dbid) {
    dbresult_t* result = dbqueryf(dbid,
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
