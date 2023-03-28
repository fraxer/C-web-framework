#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "../log/log.h"
#include "dbresult.h"
#include "postgresql.h"

void postgresql_connection_free(dbconnection_t*);
void postgresql_send_query(dbresult_t*, dbconnection_t*, const char*);
PGconn* postgresql_connect(postgresqlconfig_t*);

postgresqlconfig_t* postgresql_config_create() {
    postgresqlconfig_t* config = (postgresqlconfig_t*)malloc(sizeof(postgresqlconfig_t));

    config->base.free = postgresql_config_free;
    config->base.connection_create = postgresql_connection_create;
    config->host = NULL;
    config->current_host = NULL;
    config->dbname = NULL;
    config->user = NULL;
    config->password = NULL;
    config->connection_timeout = 0;

    return config;
}

void postgresql_config_free(void* arg) {
    if (arg == NULL) return;

    postgresqlconfig_t* config = (postgresqlconfig_t*)arg;

    db_host_free(config->host);
    config->host = NULL;

    if (config->dbname) free(config->dbname);
    if (config->user) free(config->user);
    if (config->password) free(config->password);

    free(config);
}

void postgresql_free(db_t* db) {
    db_free(db);
}

dbconnection_t* postgresql_connection_create(dbconfig_t* config) {
    postgresqlconfig_t* pgconfig = (postgresqlconfig_t*)config;

    postgresqlconnection_t* connection = (postgresqlconnection_t*)malloc(sizeof(postgresqlconnection_t));

    if (connection == NULL) return NULL;

    connection->base.locked = 0;
    connection->base.next = NULL;
    connection->base.free = postgresql_connection_free;
    connection->base.send_query = postgresql_send_query;

    void* host_address = pgconfig->current_host;

    while (1) {
        connection->connection = postgresql_connect(pgconfig);

        int status = PQstatus(connection->connection);

        if (status == CONNECTION_OK) break;

        log_error("Postgresql error: %s\n", PQerrorMessage(connection->connection));

        if (host_address == pgconfig->current_host) {
            postgresql_connection_free((dbconnection_t*)connection);
            return NULL;
        }
        
        PQfinish(connection->connection);
    }

    return (dbconnection_t*)connection;
}

const char* postgresql_host(postgresqlconfig_t* config) {
    return config->current_host->ip;
}

int postgresql_port(postgresqlconfig_t* config) {
    return config->current_host->port;
}

void postgresql_next_host(postgresqlconfig_t* config) {
    if (config->current_host->next != NULL) {
        config->current_host = config->current_host->next;
        return;
    }

    config->current_host = config->host;
}

void postgresql_connection_free(dbconnection_t* connection) {
    if (connection == NULL) return;

    postgresqlconnection_t* conn = (postgresqlconnection_t*)connection;

    PQfinish(conn->connection);
    free(conn);
}

void postgresql_send_query(dbresult_t* result, dbconnection_t* connection, const char* string) {
    postgresqlconnection_t* pgconnection = (postgresqlconnection_t*)connection;

    if (!PQsendQuery(pgconnection->connection, string)) {
        log_error("Postgresql error: %s", PQerrorMessage(pgconnection->connection));
        result->error_code = 1;
        result->error_message = "Postgresql error: connection error";
        return;
    }

    PGresult* res = NULL;
    dbresultquery_t* query_last = NULL;

    while ((res = PQgetResult(pgconnection->connection))) {
        ExecStatusType status = PQresultStatus(res);

        if (status == PGRES_TUPLES_OK || status == PGRES_SINGLE_TUPLE) {
            int cols = PQnfields(res);
            int rows = PQntuples(res);

            dbresultquery_t* query = dbresult_query_create(rows, cols);

            if (query == NULL) {
                result->ok = 0;
                result->error_message = "Out of memory";
                goto clear;
            }

            if (query_last) {
                query_last->next = query;
            }
            query_last = query;

            if (result->query == NULL) {
                result->query = query;
                result->current = query;
            }

            for (int col = 0; col < cols; col++) {
                dbresult_query_field_insert(query, PQfname(res, col), col);
            }

            for (int row = 0; row < rows; row++) {
                for (int col = 0; col < cols; col++) {
                    size_t length = PQgetlength(res, row, col);
                    const char* value = PQgetvalue(res, row, col);

                    dbresult_query_table_insert(query, value, length, row, col);
                }
            }

            result->ok = 1;
        }
        else if (status == PGRES_FATAL_ERROR) {
            log_error("Postgresql Fatal error: %s", PQresultErrorMessage(res));
            result->ok = 0;
            result->error_code = 1;
            result->error_message = "Postgresql error: fatal error";
        }
        else if (status == PGRES_NONFATAL_ERROR) {
            log_error("Postgresql Nonfatal error: %s", PQresultErrorMessage(res));
            result->ok = 0;
            result->error_code = 1;
            result->error_message = "Postgresql error: non fatal error";
        }
        else if (status == PGRES_EMPTY_QUERY) {
            log_error("Postgresql Empty query: %s", PQresultErrorMessage(res));
            result->ok = 0;
            result->error_message = "Postgresql error: empty query";
        }
        else if (status == PGRES_BAD_RESPONSE) {
            log_error("Postgresql Bad response: %s", PQresultErrorMessage(res));
            result->ok = 0;
            result->error_code = 1;
            result->error_message = "Postgresql error: bad response";
        }
        else if (status == PGRES_COMMAND_OK) {
            result->ok = 1;
        }

        clear:

        PQclear(res);
    }

    return;
}

PGconn* postgresql_connect(postgresqlconfig_t* config) {
    char string[1024] = {0};

    void append(char* string, const char* key, const char* value) {
        strcat(string, key);
        strcat(string, "=");
        strcat(string, value);
        strcat(string, " ");
    }

    append(string, "host", postgresql_host(config));

    sprintf(&string[strlen(string)], "port=%d ", postgresql_port(config));

    append(string, "dbname", config->dbname);
    append(string, "user", config->user);
    append(string, "password", config->password);

    sprintf(&string[strlen(string)], "connect_timeout=%d ", config->connection_timeout);

    postgresql_next_host(config);

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
            config->current_host = config->host;

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
            // log_error("Error: Fill database config\n");
            goto failed;
        }
    }

    database->config = (dbconfig_t*)config;

    result = database;

    failed:

    if (result == NULL) {
        db_free(database);
    }

    return result;
}
