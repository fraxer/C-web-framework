#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "../log/log.h"
#include "dbresult.h"
#include "postgresql.h"

void postgresql_connection_free(dbconnection_t*);
void postgresql_send_query(dbresult_t*, dbconnection_t*, const char*);
void postgresql_append_string(char*, const char*, const char*);
PGconn* postgresql_connect(dbhosts_t*);

postgresqlhost_t* postgresql_host_create() {
    postgresqlhost_t* host = malloc(sizeof *host);

    host->base.free = postgresql_host_free;
    host->base.next = NULL;
    host->port = 0;
    host->ip = NULL;
    host->dbname = NULL;
    host->user = NULL;
    host->password = NULL;
    host->connection_timeout = 0;

    return host;
}

void postgresql_host_free(void* arg) {
    if (arg == NULL) return;

    postgresqlhost_t* host = arg;

    if (host->ip) free(host->ip);
    if (host->dbname) free(host->dbname);
    if (host->user) free(host->user);
    if (host->password) free(host->password);
    host->port = 0;
    host->connection_timeout = 0;
    host->base.next = NULL;

    free(host);
}

void postgresql_free(db_t* db) {
    db_free(db);
}

dbconnection_t* postgresql_connection_create(dbhosts_t* hosts) {
    postgresqlconnection_t* connection = (postgresqlconnection_t*)malloc(sizeof(postgresqlconnection_t));

    if (connection == NULL) return NULL;

    connection->base.locked = 0;
    connection->base.next = NULL;
    connection->base.free = postgresql_connection_free;
    connection->base.send_query = postgresql_send_query;

    void* host_address = hosts->current_host;

    while (1) {
        connection->connection = postgresql_connect(hosts);

        int status = PQstatus(connection->connection);

        if (status == CONNECTION_OK) break;

        log_error("Postgresql error: %s\n", PQerrorMessage(connection->connection));

        if (host_address == hosts->current_host) {
            postgresql_connection_free((dbconnection_t*)connection);
            return NULL;
        }
        
        PQfinish(connection->connection);
    }

    return (dbconnection_t*)connection;
}

void postgresql_next_host(dbhosts_t* hosts) {
    if (hosts->current_host->next != NULL) {
        hosts->current_host = hosts->current_host->next;
        return;
    }

    hosts->current_host = hosts->host;
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
        result->error_message = "Postgresql error: connection error";
        return;
    }

    result->ok = 1;

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
        }
        else if (status == PGRES_FATAL_ERROR) {
            log_error("Postgresql Fatal error: %s", PQresultErrorMessage(res));
            result->ok = 0;
            result->error_message = "Postgresql error: fatal error";
        }
        else if (status == PGRES_NONFATAL_ERROR) {
            log_error("Postgresql Nonfatal error: %s", PQresultErrorMessage(res));
            result->ok = 0;
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
            result->error_message = "Postgresql error: bad response";
        }

        clear:

        PQclear(res);
    }

    return;
}

void postgresql_append_string(char* string, const char* key, const char* value) {
    strcat(string, key);
    strcat(string, "=");
    strcat(string, value);
    strcat(string, " ");
}

PGconn* postgresql_connect(dbhosts_t* hosts) {
    postgresqlhost_t* host = (postgresqlhost_t*)hosts->current_host;
    char string[1024] = {0};

    postgresql_append_string(string, "host", host->ip);
    sprintf(&string[strlen(string)], "port=%d ", host->port);
    postgresql_append_string(string, "dbname", host->dbname);
    postgresql_append_string(string, "user", host->user);
    postgresql_append_string(string, "password", host->password);
    sprintf(&string[strlen(string)], "connect_timeout=%d ", host->connection_timeout);

    postgresql_next_host(hosts);

    return PQconnectdb(string);
}

db_t* postgresql_load(const char* database_id, const jsmntok_t* token_array) {
    db_t* result = NULL;
    db_t* database = db_create(database_id);
    if (database == NULL) goto failed;

    database->hosts = db_hosts_create(postgresql_connection_create);
    if (database->hosts == NULL) goto failed;

    enum fields { PORT = 0, IP, DBNAME, USER, PASSWORD, CONNECTION_TIMEOUT, FIELDS_COUNT };
    int finded_fields[FIELDS_COUNT] = {0};
    dbhost_t* host_last = NULL;

    for (jsmntok_t* token_object = token_array->child; token_object; token_object = token_object->sibling) {
        postgresqlhost_t* host = postgresql_host_create();
        
        for (jsmntok_t* token = token_object->child; token; token = token->sibling) {
            const char* key = jsmn_get_value(token);

            if (strcmp(key, "port") == 0) {
                finded_fields[PORT] = 1;

                const char* value = jsmn_get_value(token->child);

                host->port = atoi(value);
            }
            else if (strcmp(key, "ip") == 0) {
                finded_fields[IP] = 1;

                const char* value = jsmn_get_value(token->child);

                host->ip = (char*)malloc(strlen(value) + 1);

                if (host->ip == NULL) goto failed;

                strcpy(host->ip, value);
            }
            else if (strcmp(key, "dbname") == 0) {
                finded_fields[DBNAME] = 1;

                const char* value = jsmn_get_value(token->child);

                host->dbname = (char*)malloc(strlen(value) + 1);

                if (host->dbname == NULL) goto failed;

                strcpy(host->dbname, value);
            }
            else if (strcmp(key, "user") == 0) {
                finded_fields[USER] = 1;

                const char* value = jsmn_get_value(token->child);

                host->user = (char*)malloc(strlen(value) + 1);

                if (host->user == NULL) goto failed;

                strcpy(host->user, value);
            }
            else if (strcmp(key, "password") == 0) {
                finded_fields[PASSWORD] = 1;

                const char* value = jsmn_get_value(token->child);

                host->password = (char*)malloc(strlen(value) + 1);

                if (host->password == NULL) goto failed;

                strcpy(host->password, value);
            }
            else if (strcmp(key, "connection_timeout") == 0) {
                finded_fields[CONNECTION_TIMEOUT] = 1;

                const char* value = jsmn_get_value(token->child);

                host->connection_timeout = atoi(value);
            }
        }

        if (database->hosts->host == NULL) {
            database->hosts->host = (dbhost_t*)host;
            database->hosts->current_host = (dbhost_t*)host;
        }
        if (host_last != NULL) {
            host_last->next = (dbhost_t*)host;
        }
        host_last = (dbhost_t*)host;

        for (int i = 0; i < FIELDS_COUNT; i++) {
            if (finded_fields[i] == 0) {
                log_error("Error: Fill postgresql config\n");
                goto failed;
            }
        }
    }

    result = database;

    failed:

    if (result == NULL) {
        db_free(database);
    }

    return result;
}
