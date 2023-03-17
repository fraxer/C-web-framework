#ifndef __DATABASE__
#define __DATABASE__

#include <stdatomic.h>
#include "../jsmn/jsmn.h"

typedef enum transaction_level {
    READ_UNCOMMITTED,
    READ_COMMITTED,
    REPEATABLE_READ,
    SERIALIZABLE
} transaction_level_e;

typedef struct dbhost {
    int port;
    char* ip;
    struct dbhost* next;
} dbhost_t;

typedef struct db_table_cell {
    int length;
    char* value;
} db_table_cell_t;

typedef struct dbresultquery {
    int rows;
    int cols;
    int current_row;
    int current_col;

    db_table_cell_t** fields;
    db_table_cell_t** table;

    struct dbresultquery* next;
} dbresultquery_t;

typedef struct dbresult {
    int ok;
    int error_code;
    const char* error_message;

    dbresultquery_t* query;
    dbresultquery_t* current;
} dbresult_t;

typedef struct dbconnection {
    atomic_bool locked;
    struct dbconnection* next;
    void(*free)(struct dbconnection*);
    dbresult_t(*send_query)(struct dbconnection*, const char*);
} dbconnection_t;

typedef struct dbconfig {
    void(*free)(void*);
    dbconnection_t*(*connection_create)(struct dbconfig*);
} dbconfig_t;

typedef struct dbinstance {
    int ok;
    int permission;
    atomic_bool* lock_connection;
    dbconfig_t* config;
    dbconnection_t** connection;
    dbconnection_t*(*connection_create)(struct dbconfig*);
} dbinstance_t;

typedef struct db {
    atomic_bool lock_connection;
    const char* id;
    dbconfig_t* config;
    dbconnection_t* connection;
    struct db* next;
} db_t;

db_t* db_alloc();

db_t* db_create(const char*);

dbhost_t* db_host_create();

void db_free(db_t*);

void db_host_free(dbhost_t*);

void db_cell_free(db_table_cell_t*);

dbconnection_t* db_connection_find(dbconnection_t*);

void db_connection_append(dbinstance_t*, dbconnection_t*);

void db_connection_pop(dbinstance_t*, dbconnection_t*);

int db_connection_trylock(dbconnection_t*);

int db_connection_lock(dbconnection_t*);

void db_connection_unlock(dbconnection_t*);

void db_connection_free(dbconnection_t*);

dbhost_t* db_hosts_load(const jsmntok_t* token_array);

#endif