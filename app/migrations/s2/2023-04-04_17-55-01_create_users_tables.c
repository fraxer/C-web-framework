#include <stdlib.h>

#include "dbquery.h"
#include "dbresult.h"

int up(dbinstance_t* dbinst) {
    dbresult_t* result = dbqueryf(dbinst, 
        "CREATE TABLE user"
        "("
            "id    int          NOT NULL PRIMARY KEY,"
            "email varchar(100) NOT NULL DEFAULT '',"
            "name  varchar(100) NOT NULL DEFAULT ''"
        ")"
    );

    int res = dbresult_ok(result);

    dbresult_free(result);

    return res;
}

int down(dbinstance_t* dbinst) {
    dbresult_t* result = dbqueryf(dbinst, "DROP TABLE users");

    int res = dbresult_ok(result);

    dbresult_free(result);

    return res;
}