#include <stdlib.h>
#include <stddef.h>
#include "postgresql.h"

postgresqlconfig_t* postgresql_config_create() {
    postgresqlconfig_t* config = (postgresqlconfig_t*)malloc(sizeof(postgresqlconfig_t));

    config->base.free = postgresql_config_free;
    config->base.connection_create = postgresql_connection_create;
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

void postgresql_free(db_t* db) {
    // if (db->host) free(db->host);
    // db->host = NULL;

    // if (db->dbname) free(db->dbname);
    // db->dbname = NULL;

    // if (db->user) free(db->user);
    // db->user = NULL;

    // if (db->password) free(db->password);
    // db->password = NULL;

    // db->driver = NONE;
    // db->port = 0;
    // db->connection_timeout = 0;

    // connection_t* connections;

    free(db);
}

dbconnection_t* postgresql_connection_create(dbconfig_t* config) {
    postgresqlconfig_t* pgconfig = (postgresqlconfig_t*)config;

    return (dbconnection_t*)malloc(sizeof(dbconnection_t));
}
