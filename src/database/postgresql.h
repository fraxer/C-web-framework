#ifndef __POSTGRESQL__
#define __POSTGRESQL__

#include <libpq-fe.h>
#include "../jsmn/jsmn.h"
#include "database.h"

typedef struct postgresqlhost {
    dbhost_t base;
    int port;
    char* ip;
    char* dbname;
    char* user;
    char* password;
    int connection_timeout;
} postgresqlhost_t;

typedef struct postgresqlconnection {
    dbconnection_t base;
    PGconn* connection;
} postgresqlconnection_t;

postgresqlhost_t* postgresql_host_create();

void postgresql_host_free(void*);

dbconnection_t* postgresql_connection_create(dbhosts_t*);

void postgresql_next_host(dbhosts_t*);

db_t* postgresql_load(const char*, const jsmntok_t*);

#endif