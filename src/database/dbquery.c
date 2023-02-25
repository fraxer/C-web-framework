#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "dbquery.h"
    #include <stdio.h>

dbinstance_t db_instance(db_t* db, dbperms_e permission, const char* dbid) {
    dbinstance_t inst = {
        .ok = 0,
        .config = NULL,
        .lock_connection = 0,
        .connection = NULL
    };

    while (db) {
        if (strcmp(db->id, dbid) == 0) {
            inst.ok = 1;
            inst.config = db->config;
            inst.connection_create = db->config->connection_create;

            if (permission == READ) {
                inst.lock_connection = &db->lock_connection_read;
                inst.connection = db->connection_read;
            }
            else if (permission == WRITE) {
                inst.lock_connection = &db->lock_connection_write;
                inst.connection = db->connection_write;
            }
            
            return inst;
        }

        db = db->next;
    }

    return inst;
}

dbresult_t db_query(dbinstance_t* instance, const char* string) {
    dbresult_t result = {
        .ok = 0,
        .error_message = "database query error",
        .data = NULL
    };

    dbconnection_t* connection = db_find_free_connection(instance->connection);

    if (connection == NULL) {
        connection = instance->connection_create(instance->config);

        if (connection == NULL) {
            return result;
        }

        db_connection_trylock(connection);
        db_connection_append(instance, connection);
    }

    result = connection->send_query(connection, string);

    db_connection_unlock(connection);

    return result;
}

dbresult_t db_begin(dbinstance_t* instance, transaction_level_e level) {
    return db_query(instance, "level");
}

dbresult_t db_commit(dbinstance_t* instance) {
    return db_query(instance, "commit");
}

dbresult_t db_rollback(dbinstance_t* instance) {
    return db_query(instance, "rollback");
}

void db_result_free(dbresult_t* result) {
    result->ok = 0;
    result->error_message = NULL;

    if (result->data) free(result->data);
    result->data = NULL;
}
