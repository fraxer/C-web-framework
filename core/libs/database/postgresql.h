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

db_t* postgresql_load(const char* database_id, const jsontok_t* token_array);

#endif