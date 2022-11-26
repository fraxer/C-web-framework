#include <stdlib.h>
#include <stddef.h>
#include "database.h"

database_t* database_alloc() {
    return (database_t*)malloc(sizeof(database_t));
}

database_t* database_create() {
    database_t* database = database_alloc();

    if (database == NULL) return NULL;

    database->host = NULL;
    database->port = 0;
    database->dbname = NULL;
    database->user = NULL;
    database->password = NULL;
    database->driver = NONE;
    database->connection_timeout = 0;
    database->connections = NULL;

    return database;
}

void database_free(database_t* database) {
    if (database->host) free(database->host);
    database->host = NULL;

    if (database->dbname) free(database->dbname);
    database->dbname = NULL;

    if (database->user) free(database->user);
    database->user = NULL;

    if (database->password) free(database->password);
    database->password = NULL;

    database->driver = NONE;
    database->port = 0;
    database->connection_timeout = 0;

    // connection_t* connections;

    free(database);
}

database_connection_t* database_connection_create(database_t* database) {
    return NULL;
}
