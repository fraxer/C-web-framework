#include <stdlib.h>
#include <stdio.h>

#include "dbquery.h"
#include "dbresult.h"

int up(const char* dbid) {
    dbresult_t* result = dbquery(dbid,
        "CREATE TABLE public.role"
        "("
            "id    bigserial    NOT NULL PRIMARY KEY,"
            "name  varchar(100) NOT NULL DEFAULT ''"
        ")"
    , NULL);

    int res = dbresult_ok(result);

    dbresult_free(result);

    return res;
}
