#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "database.h"

database_t* database_alloc() {
    return (database_t*)malloc(sizeof(database_t));
}

const char* database_alloc_id(const char* database_id) {
    char* string = (char*)malloc(strlen(database_id) + 1);

    if (string == NULL) return NULL;

    return strcpy(string, database_id);
}

database_t* database_create(const char* database_id) {
    database_t* result = NULL;
    database_t* database = database_alloc();

    if (database == NULL) return NULL;

    database->id = database_alloc_id(database_id);

    if (database->id == NULL) goto failed;

    database->config = NULL;
    database->connection = NULL;
    database->next = NULL;

    result = database;

    failed:

    if (result == NULL) {
        free(database);
    }

    return result;
}

databasehost_t* database_host_create() {
    databasehost_t* host = (databasehost_t*)malloc(sizeof(databasehost_t));

    host->ip = NULL;
    host->port = 0;
    host->read = 0;
    host->write = 0;
    host->next = NULL;

    return host;
}

void database_free(database_t* database) {
    while (database) {
        database_t* next = database->next;

        if (database->config != NULL) {
            database->config->free(database->config);
        }
        if (database->connection != NULL) {
            database->connection->free(database->connection);
        }
        free((void*)database->id);
        free(database);

        database = next;
    }
}

void database_host_free(databasehost_t* host) {
    while (host) {
        databasehost_t* next = host->next;

        host->port = 0;
        host->read = 0;
        host->write = 0;
        if (host->ip) free(host->ip);
        free(host);

        host = next;
    }
}

databaseresult_t* db_query(const char* string) {
    return NULL;
}

databaseresult_t* db_cquery(databaseconfig_t* config, const char* string) {
    return NULL;
}

databaseresult_t* db_begin(transaction_level_e level) {
    return NULL;
}

databaseresult_t* db_cbegin(databaseconfig_t* config, transaction_level_e level) {
    return NULL;
}

databaseresult_t* db_commit() {
    return NULL;
}

databaseresult_t* db_ccommit(databaseconfig_t* config) {
    return NULL;
}

databaseresult_t* db_rollback() {
    return NULL;
}

databaseresult_t* db_crollback(databaseconfig_t* config) {
    return NULL;
}