#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "appconfig.h"
#include "str.h"
#include "log.h"
#include "dbquery.h"

dbinstance_t* dbinstance(const char* dbid) {
    array_t* dbs = appconfig()->databases;
    for (size_t i = 0; i < array_size(dbs); i++) {
        db_t* db = array_get(dbs, i);

        if (strcmp(db->id, dbid) == 0) {
            dbinstance_t* inst = malloc(sizeof * inst);
            if (inst == NULL) return NULL;

            memset(inst, 0, sizeof * inst);

            // divide dbid and get host_id or NULL
            dbhost_t* host = db_host_find(db->hosts, NULL);
            if (host == NULL) {
                // free memory
                log_error("dbhost not found\n");
                return NULL;
            }

            host_connections_lock(host);
            dbconnection_t* connection = db_connection_find(host->connections);
            host_connections_unlock(host);

            if (connection == NULL) {
                connection = host->connection_create(host);

                if (connection == NULL) {
                    // free memory
                    log_error("dbconnection not found\n");
                    return NULL;
                }

                host_connections_lock(host);
                array_push_back(host->connections, array_create_pointer(connection, connection->free));
                host_connections_unlock(host);
            }

            inst->grammar = &host->grammar;
            inst->connection = connection;

            return inst;
        }
    }

    return NULL;
}

void dbinstance_free(dbinstance_t* instance) {
    free(instance);
}

dbresult_t* dbquery(dbinstance_t* instance, const char* format, ...) {
    va_list args;
    va_start(args, format);
    size_t string_length = vsnprintf(NULL, 0, format, args);
    va_end(args);

    char* string = malloc(string_length + 1);
    if (string == NULL) return NULL;

    va_start(args, format);
    vsnprintf(string, string_length + 1, format, args);
    va_end(args);

    dbconnection_t* connection = instance->connection;
    dbresult_t* result = connection->query(connection, string);

    free(string);

    return result;
}

dbresult_t dbtable_exist(dbinstance_t* instance, const char* table) {
    dbresult_t result = {
        .ok = 0,
        .error = "",
        .query = NULL,
        .current = NULL
    };

    // if (instance->table_exist_sql == NULL) return result;

    // const char* sql = instance->table_exist_sql(table);
    // if (sql == NULL) return result;

    // result = dbquery(instance, sql);

    // free((void*)sql);

    return result;
}

dbresult_t dbtable_migration_create(dbinstance_t* instance) {
    dbresult_t result = {
        .ok = 0,
        .error = "",
        .query = NULL,
        .current = NULL
    };

    // if (instance->table_migration_create_sql == NULL) return result;

    // const char* sql = instance->table_migration_create_sql();
    // if (sql == NULL) return result;

    // result = dbquery(instance, sql);

    // free((void*)sql);

    return result;
}

dbresult_t* dbbegin(dbinstance_t* instance, transaction_level_e level) {
    (void)level;
    return dbquery(instance, "level");
}

dbresult_t* dbcommit(dbinstance_t* instance) {
    return dbquery(instance, "commit");
}

dbresult_t* dbrollback(dbinstance_t* instance) {
    return dbquery(instance, "rollback");
}

dbresult_t* dbinsert(dbinstance_t* instance, const char* table, array_t* params) {
    dbconnection_t* connection = instance->connection;

    char* sql = instance->grammar->compile_insert(connection, table, params);
    if (sql == NULL) return NULL;

    dbresult_t* result = connection->query(connection, sql);

    free(sql);

    return result;
}
