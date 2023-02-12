#include <stdlib.h>
#include <stddef.h>
#include "postgresql.h"

postgresqlconfig_t* postgresql_config_create() {
    postgresqlconfig_t* config = (postgresqlconfig_t*)malloc(sizeof(postgresqlconfig_t));

    config->base.free = postgresql_config_free;
    config->host = NULL;
    config->dbname = NULL;
    config->user = NULL;
    config->password = NULL;
    config->charset = NULL;
    config->collation = NULL;
    config->connection_timeout = 0;

    return config;
}

void postgresql_config_free(void* config) {

}

void postgresql_free(database_t* database) {
    // if (database->host) free(database->host);
    // database->host = NULL;

    // if (database->dbname) free(database->dbname);
    // database->dbname = NULL;

    // if (database->user) free(database->user);
    // database->user = NULL;

    // if (database->password) free(database->password);
    // database->password = NULL;

    // database->driver = NONE;
    // database->port = 0;
    // database->connection_timeout = 0;

    // connection_t* connections;

    free(database);
}

// database_connection_t* database_connection_create(database_t* database) {
//     return NULL;
// }
