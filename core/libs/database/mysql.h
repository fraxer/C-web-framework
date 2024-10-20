#ifndef __CMYSQL__
#define __CMYSQL__

#include <mysql/mysql.h>

#include "json.h"
#include "database.h"

typedef struct myhost {
    dbhost_t base;
    char* dbname;
    char* user;
    char* password;
} myhost_t;

typedef struct myconnection {
    dbconnection_t base;
    MYSQL* connection;
} myconnection_t;

db_t* my_load(const char* database_id, const jsontok_t* token_array);

#endif