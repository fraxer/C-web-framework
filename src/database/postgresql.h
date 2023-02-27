#ifndef __POSTGRESQL__
#define __POSTGRESQL__

#include <libpq-fe.h>
#include "../jsmn/jsmn.h"
#include "database.h"

typedef struct postgresqlconfig {
    dbconfig_t base;
    dbhost_t* host;
    char* dbname;
    char* user;
    char* password;
    int connection_timeout;
} postgresqlconfig_t;

typedef struct postgresqlconnection {
    dbconnection_t base;
    PGconn* connection;
} postgresqlconnection_t;

postgresqlconfig_t* postgresql_config_create();

void postgresql_config_free(void*);

dbconnection_t* postgresql_connection_create(dbconfig_t*);

// void db_free(db_t*);

db_t* postgresql_load(const char*, const jsmntok_t* token_object);

#endif