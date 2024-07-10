#ifndef __POSTGRESQL__
#define __POSTGRESQL__

#include <postgresql/libpq-fe.h>

#include "json.h"
#include "database.h"

typedef struct postgresqlhost {
    dbhost_t base;
    char* dbname;
    char* user;
    char* password;
    int connection_timeout;
} postgresqlhost_t;

typedef struct postgresqlconnection {
    dbconnection_t base;
    PGconn* connection;
} postgresqlconnection_t;

postgresqlhost_t* postgresql_host_create(void);
void postgresql_host_free(void*);
dbconnection_t* postgresql_connection_create(dbhosts_t* hosts);
dbconnection_t* postgresql_connection_create_manual(dbhosts_t* hosts);
const char* postgresql_table_exist_sql(const char* table);
const char* postgresql_table_migration_create_sql(void);
db_t* postgresql_load(const char* database_id, const jsontok_t* token_array);

#endif