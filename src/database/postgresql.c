#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "../log/log.h"
#include "postgresql.h"

void postgresql_connection_free(dbconnection_t*);
dbresult_t send_query(dbconnection_t*, const char*);
PGconn* postgresql_connect(postgresqlconfig_t*);

postgresqlconfig_t* postgresql_config_create() {
    postgresqlconfig_t* config = (postgresqlconfig_t*)malloc(sizeof(postgresqlconfig_t));

    config->base.free = postgresql_config_free;
    config->base.connection_create = postgresql_connection_create;
    config->host = NULL;
    config->dbname = NULL;
    config->user = NULL;
    config->password = NULL;
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

    postgresqlconnection_t* connection = (postgresqlconnection_t*)malloc(sizeof(postgresqlconnection_t));

    if (connection == NULL) return NULL;

    connection->base.locked = 0;
    connection->base.next = NULL;
    connection->base.free = postgresql_connection_free;
    connection->base.send_query = send_query;
    connection->connection = postgresql_connect(pgconfig);

    if (PQstatus(connection->connection) != CONNECTION_OK) {
        log_error("Postgresql error: %s\n", PQerrorMessage(connection->connection));
        postgresql_connection_free((dbconnection_t*)connection);
        return NULL;
    }

    return (dbconnection_t*)connection;
}

void postgresql_connection_free(dbconnection_t* connection) {

}

dbresult_t send_query(dbconnection_t* connection, const char* string) {
    postgresqlconnection_t* pgconnection = (postgresqlconnection_t*)connection;

    dbresult_t result = {
        .ok = 0,
        .error_code = 0,
        .error_message = "Postgresql query error",
        .data = NULL
    };

    if (!PQsendQuery(pgconnection->connection, string)){
        log_error("Postgresql error: %s", PQerrorMessage(pgconnection->connection));
        return result;
    }

    PGresult* res = NULL;

    while (res = PQgetResult(pgconnection->connection)) {
        ExecStatusType status = PQresultStatus(res);

        if (status == PGRES_TUPLES_OK || status == PGRES_SINGLE_TUPLE) {
            int cols = PQnfields(res);
            int rows = PQntuples(res);

            // syslog(LOG_INFO, "[INFO][model/model.cpp][queryNew] %d, %d\n", num_cols, num_rows);

            for (int row = 0; row < rows; row++) {
                // syslog(LOG_INFO, "[INFO][model/model.cpp][queryNew] %s\n", PQfname(res, col));

                for (int col = 0; col < cols; col++) {
                    // syslog(LOG_INFO, "[INFO][model/model.cpp][queryNew] %s\n", PQgetvalue(res, row, col));
                    // PQgetvalue(res, row, col); // get data

                    int value_length = PQgetlength(res, row, col);
                    char* value = (char*)malloc(value_length + 1);

                    if (value == NULL) {
                        result.ok = 0;
                        result.error_message = "Postgresql error: out of memory";
                        return result;
                    }

                    strcpy(value, PQgetvalue(res, row, col));

                    // result.table

                    // printf("%s\n", value);
                }

                // append data to list
            }

            // result.ok = 1;
        }
        else if (status == PGRES_FATAL_ERROR) {
            log_error("Postgresql Fatal error: %s", PQresultErrorMessage(res));
        }
        else if (status == PGRES_NONFATAL_ERROR) {
            log_error("Postgresql Nonfatal error: %s", PQresultErrorMessage(res));
        }
        else if (status == PGRES_EMPTY_QUERY) {
            log_error("Postgresql Empty query: %s", PQresultErrorMessage(res));
        }
        else if (status == PGRES_BAD_RESPONSE) {
            log_error("Postgresql Bad response: %s", PQresultErrorMessage(res));
        }
        else if (status == PGRES_COMMAND_OK) {
            // result.ok = 1;
        }

        PQclear(res);
    }

    printf("%d\n", result.ok);

    return result;
}

PGconn* postgresql_connect(postgresqlconfig_t* config) {
    char string[1024] = {0};

    void append(char* string, char* key, char* value) {
        strcat(string, key);
        strcat(string, "=");
        strcat(string, value);
        strcat(string, " ");
    }

    append(string, "host", "127.0.0.1");
    append(string, "port", "5432");
    append(string, "dbname", config->dbname);
    append(string, "user", config->user);
    append(string, "password", config->password);

    char connection_timeout[10] = {0};
    sprintf(connection_timeout, "%d", config->connection_timeout);

    append(string, "connect_timeout", connection_timeout);

    return PQconnectdb(string);
}


db_t* postgresql_load(const char* database_id, const jsmntok_t* token_object) {
    db_t* result = NULL;
    db_t* database = db_create(database_id);

    if (database == NULL) goto failed;

    postgresqlconfig_t* config = postgresql_config_create();

    if (config == NULL) goto failed;

    enum fields { HOSTS = 0, DBNAME, USER, PASSWORD, CONNECTION_TIMEOUT, FIELDS_COUNT };

    int finded_fields[FIELDS_COUNT] = {0};

    for (jsmntok_t* token = token_object->child; token; token = token->sibling) {
        const char* key = jsmn_get_value(token);

        if (strcmp(key, "hosts") == 0) {
            finded_fields[HOSTS] = 1;

            config->host = db_hosts_load(token->child);

            if (config->host == NULL) goto failed;
        }
        else if (strcmp(key, "dbname") == 0) {
            finded_fields[DBNAME] = 1;

            const char* value = jsmn_get_value(token->child);

            config->dbname = (char*)malloc(strlen(value) + 1);

            if (config->dbname == NULL) goto failed;

            strcpy(config->dbname, value);
        }
        else if (strcmp(key, "user") == 0) {
            finded_fields[USER] = 1;

            const char* value = jsmn_get_value(token->child);

            config->user = (char*)malloc(strlen(value) + 1);

            if (config->user == NULL) goto failed;

            strcpy(config->user, value);
        }
        else if (strcmp(key, "password") == 0) {
            finded_fields[PASSWORD] = 1;

            const char* value = jsmn_get_value(token->child);

            config->password = (char*)malloc(strlen(value) + 1);

            if (config->password == NULL) goto failed;

            strcpy(config->password, value);
        }
        else if (strcmp(key, "connection_timeout") == 0) {
            finded_fields[CONNECTION_TIMEOUT] = 1;

            const char* value = jsmn_get_value(token->child);

            config->connection_timeout = atoi(value);
        }
    }

    for (int i = 0; i < FIELDS_COUNT; i++) {
        if (finded_fields[i] == 0) {
            log_error("Error: Fill database config\n");
            goto failed;
        }
    }

    database->config = (dbconfig_t*)config;

    // printf("dbname %s, user %s, pass %s, timeout %d, charset %s, collation %s\n", config->dbname, config->user, config->password, config->connection_timeout, config->charset, config->collation);

    result = database;

    failed:

    if (result == NULL) {
        db_free(database);
    }

    return result;
}