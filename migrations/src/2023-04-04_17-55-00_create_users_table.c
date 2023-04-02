#include <stdlib.h>
#include <stdio.h>
#include "../../src/database/dbquery.h"
#include "../../src/database/dbresult.h"

const char* server() { return "s1"; }
const char* db() { return "postresql"; }

int up(dbinstance_t* dbinst) {
    dbresult_t result = dbquery(dbinst, "CREATE TABLE users");

    if (!dbresult_ok(&result)) {
        printf("%s\n", dbresult_error_message(&result));
        dbresult_free(&result);
        return -1;
    }

    dbresult_free(&result);
    return 0;
}

int down(dbinstance_t* dbinst) {
    dbresult_t result = dbquery(dbinst, "DROP TABLE users");

    if (!dbresult_ok(&result)) {
        printf("%s\n", dbresult_error_message(&result));
        dbresult_free(&result);
        return -1;
    }

    dbresult_free(&result);
    return 0;
}