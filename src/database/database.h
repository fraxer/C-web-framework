#ifndef __DATABASE__
#define __DATABASE__

typedef enum database_driver {
    NONE,
    POSTGRESQL,
    MYSQL,
} database_driver_e;

typedef enum transaction_level {
    READ_UNCOMMITTED,
    READ_COMMITTED,
    REPEATABLE_READ,
    SERIALIZABLE
} transaction_level_e;

typedef struct database_connection {
    char* host;
    struct database_connection* next;
} database_connection_t;

typedef struct database {
    char* host;
    unsigned short port;
    char* dbname;
    char* user;
    char* password;
    database_driver_e driver;
    unsigned short connection_timeout;
    database_connection_t* connections;
} database_t;

database_t* database_create();

database_connection_t* database_connection_create(database_t*);

int query();

void begin();

void begin(transaction_level_e);

void commit();

void rollback();

void database_free(database_t*);

#endif