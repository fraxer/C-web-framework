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

myhost_t* my_host_create(void);
void my_host_free(void* arg);
dbconnection_t* my_connection_create(dbhosts_t* hosts);
dbconnection_t* my_connection_create_manual(dbhosts_t* hosts);
const char* my_table_exist_sql(const char* table);
const char* my_table_migration_create_sql(void);
db_t* my_load(const char* database_id, const jsontok_t* token_array);

#endif