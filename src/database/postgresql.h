#ifndef __POSTGRESQL__
#define __POSTGRESQL__

#include "database.h"

typedef struct postgresqlconfig {
    databaseconfig_t base;
    databasehost_t* host;
    char* dbname;
    char* user;
    char* password;
    char* charset;
    char* collation;
    int connection_timeout;
} postgresqlconfig_t;

typedef struct postgresqlconnection {
    databaseconnection_t base;
    void* connection;
    pthread_mutex_t mutex;
    struct databaseconnection* next;
} postgresqlconnection_t;

postgresqlconfig_t* postgresql_config_create();

void postgresql_config_free(void*);

// databaseconnection_t* database_connection_create(postgresqlconfig_t*);

// int query(const char*);

// int cquery(dbconfig_t*, const char*);

// void begin();

// void begin(transaction_level_e);

// void commit();

// void rollback();

// void database_free(database_t*);

#endif