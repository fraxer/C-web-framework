#ifndef __DATABASE__
#define __DATABASE__

#include <stdatomic.h>

#include "array.h"
#include "json.h"

typedef enum transaction_level {
    READ_UNCOMMITTED,
    READ_COMMITTED,
    REPEATABLE_READ,
    SERIALIZABLE
} transaction_level_e;

typedef struct {
    size_t length;
    char* value;
} db_table_cell_t;

typedef struct dbresultquery {
    int rows;
    int cols;
    int current_row;
    int current_col;

    db_table_cell_t* fields;
    db_table_cell_t* table;

    struct dbresultquery* next;
} dbresultquery_t;

typedef struct dbresult {
    int ok;

    dbresultquery_t* query;
    dbresultquery_t* current;
} dbresult_t;

typedef struct dbconnection {
    pid_t thread_id;
    void(*free)(void*);
    dbresult_t*(*query)(void* connection, const char* sql);
    str_t*(*escape_identifier)(void* connection, str_t* string);
    str_t*(*escape_string)(void* connection, str_t* string);
    int(*is_active)(void* connection);
    int(*reconnect)(void* host, void* connection);
} dbconnection_t;

typedef struct dbgrammar {
    char*(*compile_table_exist)(const char* table);
    char*(*compile_table_migration_create)(const char* table);
    char*(*compile_insert)(void* connection, const char* table, array_t* params);
    char*(*compile_select)(void* connection, const char* table, array_t* columns, array_t* where);
    char*(*compile_update)(void* connection, const char* table, array_t* set, array_t* where);
    char*(*compile_delete)(void* connection, const char* table, array_t* where);
} dbgrammar_t;

typedef struct dbhost {
    void(*free)(void*);
    int port;
    char* ip;
    char* id;
    dbgrammar_t grammar;
    array_t* connections; // dbconnection_t
    atomic_bool connections_locked;
    void*(*connection_create)(void* host);
} dbhost_t;

typedef struct dbinstance {
    dbgrammar_t* grammar;
    void* connection; // dbconnection_t
} dbinstance_t;

typedef struct db_t {
    atomic_bool locked;
    int circular_index;
    char* id;
    array_t* hosts; // dbhost_t
} db_t;

db_t* db_create(const char*);
void db_free(void*);

void db_cell_free(db_table_cell_t* cell);
dbhost_t* db_host_find(db_t* db, const char* host_id);
dbconnection_t* db_connection_find(array_t* connections);
int host_connections_lock(dbhost_t* host);
void host_connections_unlock(dbhost_t* host);
void db_lock(db_t* db);
void db_unlock(db_t* db);

#endif