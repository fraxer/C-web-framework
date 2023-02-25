#ifndef __POSTGRESQL__
#define __POSTGRESQL__

#include "database.h"

typedef struct postgresqlconfig {
    dbconfig_t base;
    dbhost_t* host;
    char* dbname;
    char* user;
    char* password;
    char* charset;
    char* collation;
    int connection_timeout;
} postgresqlconfig_t;

typedef struct postgresqlconnection {
    dbconnection_t base;
    void* connection;
    pthread_mutex_t mutex;
} postgresqlconnection_t;

postgresqlconfig_t* postgresql_config_create();

void postgresql_config_free(void*);

dbconnection_t* postgresql_connection_create(dbconfig_t*);

// int query(const char*);

// int cquery(dbconfig_t*, const char*);

// void begin();

// void begin(transaction_level_e);

// void commit();

// void rollback();

// void db_free(db_t*);

#endif