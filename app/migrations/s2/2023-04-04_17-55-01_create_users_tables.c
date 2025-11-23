#include <stdlib.h>

#include "dbquery.h"
#include "dbresult.h"

int up(const char* dbid) {
    dbresult_t* result = dbqueryf(dbid, 
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
