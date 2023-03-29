#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "dbquery.h"

dbinstance_t dbinstance(db_t* db, const char* dbid) {
    dbinstance_t inst = {
        .ok = 0,
        .config = NULL,
        .connection_create = NULL,
        .lock_connection = 0,
        .connection = NULL
    };

    while (db) {
        if (strcmp(db->id, dbid) == 0) {
            inst.ok = 1;
            inst.config = db->config;
            inst.connection_create = db->config->connection_create;
            inst.lock_connection = &db->lock_connection;
            inst.connection = &db->connection;

            return inst;
        }

        db = db->next;
    }

    return inst;
}

dbresult_t dbquery(dbinstance_t* instance, const char* string) {
    dbresult_t result = {
        .ok = 0,
        .error_message = "",
        .query = NULL,
        .current = NULL
    };

    int second_try = 0;

    while (1) {
        dbconnection_t* connection = db_connection_find(*instance->connection);

        if (connection == NULL) {
            connection = instance->connection_create(instance->config);

            if (connection == NULL) {
                result.error_message = "Database query: error connection create";
                return result;
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
