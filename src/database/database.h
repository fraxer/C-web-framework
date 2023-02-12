#ifndef __DATABASE__
#define __DATABASE__

typedef enum databasedriver {
    NONE = 0,
    POSTGRESQL,
    MYSQL
} databasedriver_e;

typedef enum transaction_level {
    READ_UNCOMMITTED,
    READ_COMMITTED,
    REPEATABLE_READ,
    SERIALIZABLE
} transaction_level_e;

typedef struct databaseresult {
    // TODO
} databaseresult_t;

typedef struct databasehost {
    int read;
    int write;
    int port;
    char* ip;
    struct databasehost* next;
} databasehost_t;

typedef struct databaseconnection {
    void(*free)(void*);
} databaseconnection_t;

typedef struct databaseconfig {
    void(*free)(void*);
} databaseconfig_t;

typedef struct database { // mysql, postgreqsl, ...
    const char* id;
    databaseconfig_t* config;
    databaseconnection_t* connection;
    struct database* next;
} database_t;

// typedef struct database_chain {
//     pthread_mutex_t mutex;
//     database_t* database;
//     struct database_chain* next;
// } database_chain_t;

database_t* database_alloc();

database_t* database_create(const char*);

databasehost_t* database_host_create();

void database_free(database_t*);

// void database_chain_free(database_chain_t*);

void database_host_free(databasehost_t*);

databaseresult_t* db_query(const char*);
databaseresult_t* db_cquery(databaseconfig_t*, const char*);

databaseresult_t* db_begin(transaction_level_e);
databaseresult_t* db_cbegin(databaseconfig_t*, transaction_level_e);

databaseresult_t* db_commit();
databaseresult_t* db_ccommit(databaseconfig_t*);

databaseresult_t* db_rollback();
databaseresult_t* db_crollback(databaseconfig_t*);

#endif