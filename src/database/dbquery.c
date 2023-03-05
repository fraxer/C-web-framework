#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "dbquery.h"

dbinstance_t dbinstance(db_t* db, dbperms_e permission, const char* dbid) {
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

dbresult_t dbquery(dbinstance_t* instance, const char* string) {
    dbresult_t result = {
        .ok = 0,
        .error_code = 0,
        .error_message = "",
        .query = NULL,
        .current = NULL
    };

    dbconnection_t* connection = db_find_free_connection(instance->connection);

    if (connection == NULL) {
        connection = instance->connection_create(instance->config);

        if (connection == NULL) {
            result.error_message = "Database query: error connection create";
            return result;
        }

        db_connection_trylock(connection);
        db_connection_append(instance, connection);
    }

    result = connection->send_query(connection, string);

    db_connection_unlock(connection);

    return result;
}

dbresult_t dbbegin(dbinstance_t* instance, transaction_level_e level) {
    return dbquery(instance, "level");
}

dbresult_t dbcommit(dbinstance_t* instance) {
    return dbquery(instance, "commit");
}

dbresult_t dbrollback(dbinstance_t* instance) {
    return dbquery(instance, "rollback");
}
