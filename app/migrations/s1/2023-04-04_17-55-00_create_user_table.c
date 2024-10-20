#include <stdlib.h>

#include "dbquery.h"
#include "dbresult.h"

int up(dbinstance_t* dbinst) {
    dbresult_t* result = dbqueryf(dbinst,
        "CREATE TABLE public.user"
        "("
            "id    bigserial    NOT NULL PRIMARY KEY,"
            "email varchar(100) NOT NULL DEFAULT '',"
            "name  varchar(100) NOT NULL DEFAULT ''"
        ")"
    );

    

    int res = dbresult_ok(result);

    dbresult_free(result);

    return res;
}

int down(dbinstance_t* dbinst) {
    dbresult_t* result = dbqueryf(dbinst, "DROP TABLE public.user");

    int res = dbresult_ok(result);

    dbresult_free(result);

    return res;
}