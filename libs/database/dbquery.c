#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "dbquery.h"

dbinstance_t dbinstance(db_t* db, const char* dbid) {
    dbinstance_t inst = {
        .ok = 0,
        .hosts = NULL,
        .connection_create = NULL,
        .lock_connection = 0,
        .connection = NULL,
        .table_exist_sql = NULL,
        .table_migration_create_sql = NULL
    };

    while (db) {
        if (strcmp(db->id, dbid) == 0) {
            inst.ok = 1;
            inst.hosts = db->hosts;
            inst.connection_create = db->hosts->connection_create;
            inst.lock_connection = &db->lock_connection;
            inst.connection = &db->connection;
            inst.table_exist_sql = db->hosts->table_exist_sql;

            return inst;
        }

        db = db->next;
    }

    return inst;
}

dbresult_t dbquery(dbinstance_t* instance, const char* format, ...) {
    dbresult_t result = {
        .ok = 0,
        .error_message = "",
        .query = NULL,
        .current = NULL
    };

    va_list args;
    va_start(args, format);
    size_t string_length = vsnprintf(NULL, 0, format, args);
    va_end(args);

    char* string = malloc(string_length + 1);
    if (string == NULL) return result;

    va_start(args, format);
    vsnprintf(string, string_length + 1, format, args);
    va_end(args);

    int second_try = 0;
    while (1) {
        dbconnection_t* connection = db_connection_find(*instance->connection);

        if (connection == NULL) {
            connection = instance->connection_create(instance->hosts);

            if (connection == NULL) {
                result.error_message = "Database query: error connection create";
                goto exit;
            }

            db_connection_trylock(connection);
            db_connection_append(instance, connection);
        }

        connection->send_query(&result, connection, string);

        if (!result.ok && second_try == 0) {
            second_try = 1;
            db_connection_pop(instance, connection);
            db_connection_free(connection);

            if (*instance->connection == connection) {
                *instance->connection = NULL;
            }

            continue;
        }

        db_connection_unlock(connection);

        break;
    }

    exit:

    free(string);

    return result;
}

dbresult_t dbtable_exist(dbinstance_t* instance, const char* table) {
    dbresult_t result = {
        .ok = 0,
        .error_message = "",
        .query = NULL,
        .current = NULL
    };

    if (instance->table_exist_sql == NULL) return result;

    const char* sql = instance->table_exist_sql(table);
    if (sql == NULL) return result;

    result = dbquery(instance, sql);

    free((void*)sql);

    return result;
}

dbresult_t dbtable_migration_create(dbinstance_t* instance) {
    dbresult_t result = {
        .ok = 0,
        .error_message = "",
        .query = NULL,
        .current = NULL
    };

    if (instance->table_migration_create_sql == NULL) return result;

    const char* sql = instance->table_migration_create_sql();
    if (sql == NULL) return result;

    result = dbquery(instance, sql);

    free((void*)sql);

    return result;
}

dbresult_t dbbegin(dbinstance_t* instance, transaction_level_e level) {
    (void)level;
    return dbquery(instance, "level");
}

dbresult_t dbcommit(dbinstance_t* instance) {
    return dbquery(instance, "commit");
}

dbresult_t dbrollback(dbinstance_t* instance) {
    return dbquery(instance, "rollback");
}
