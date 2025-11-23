#include <stdlib.h>

#include "dbquery.h"
#include "dbresult.h"

int up(const char* dbid) {
    dbresult_t* result = dbqueryf(dbid,
        "CREATE TABLE public.user"
        "("
            "id         bigserial    NOT NULL PRIMARY KEY,"
            "email      varchar(100) NOT NULL DEFAULT '',"
            "name       varchar(100) NOT NULL DEFAULT '',"
            "created_at timestamp    NOT NULL DEFAULT NOW(),"
            "secret     text         NOT NULL DEFAULT ''"
        ")"
    );

    int res = dbresult_ok(result);

    dbresult_free(result);

    return res;
}
