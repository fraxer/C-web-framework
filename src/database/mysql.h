#ifndef __CMYSQL__
#define __CMYSQL__

#include <mysql.h>
#include "../json/json.h"
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

myhost_t* my_host_create();

void my_host_free(void*);

dbconnection_t* my_connection_create(dbhosts_t*);

dbconnection_t* my_connection_create_manual(dbhosts_t*);

const char* my_table_exist_sql(const char*);

const char* my_table_migration_create_sql();

db_t* my_load(const char*, const jsontok_t*);

#endif