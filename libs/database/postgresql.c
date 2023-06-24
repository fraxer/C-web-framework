#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include "log.h"
#include "dbresult.h"
#include "postgresql.h"

#define WITH_LOOP 1
#define WITHOUT_LOOP 0

dbhosts_t* postgresql_hosts_create();
void postgresql_connection_free(dbconnection_t*);
void postgresql_send_query(dbresult_t*, dbconnection_t*, const char*);
void postgresql_append_string(char*, size_t, postgresqlhost_t*);
PGconn* postgresql_connect(dbhosts_t*, int);

dbhosts_t* postgresql_hosts_create() {
    dbhosts_t* hosts = malloc(sizeof *hosts);

    hosts->host = NULL;
    hosts->current_host = NULL;
    hosts->connection_create = postgresql_connection_create;
    hosts->connection_create_manual = postgresql_connection_create_manual;
    hosts->table_exist_sql = postgresql_table_exist_sql;
    hosts->table_migration_create_sql = postgresql_table_migration_create_sql;

    return hosts;
}

postgresqlhost_t* postgresql_host_create() {
    postgresqlhost_t* host = malloc(sizeof *host);

    host->base.free = postgresql_host_free;
    host->base.migration = 0;
    host->base.port = 0;
    host->base.ip = NULL;
    host->base.next = NULL;
    host->dbname = NULL;
    host->user = NULL;
    host->password = NULL;
    host->connection_timeout = 0;

    return host;
}

void postgresql_host_free(void* arg) {
    if (arg == NULL) return;

    postgresqlhost_t* host = arg;

    if (host->base.ip) free(host->base.ip);
    if (host->dbname) free(host->dbname);
    if (host->user) free(host->user);
    if (host->password) free(host->password);
    host->base.port = 0;
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
        connection->connection = postgresql_connect(hosts, WITH_LOOP);

        if (PQstatus(connection->connection) == CONNECTION_OK) break;

        log_error("Postgresql error: %s\n", PQerrorMessage(connection->connection));

        if (host_address == hosts->current_host) {
            postgresql_connection_free((dbconnection_t*)connection);
            return NULL;
        }
        
        PQfinish(connection->connection);
    }

    return (dbconnection_t*)connection;
}

dbconnection_t* postgresql_connection_create_manual(dbhosts_t* hosts) {
    postgresqlconnection_t* connection = (postgresqlconnection_t*)malloc(sizeof(postgresqlconnection_t));

    if (connection == NULL) return NULL;

    connection->base.locked = 0;
    connection->base.next = NULL;
    connection->base.free = postgresql_connection_free;
    connection->base.send_query = postgresql_send_query;
    connection->connection = postgresql_connect(hosts, WITHOUT_LOOP);

    if (PQstatus(connection->connection) == CONNECTION_OK) {
        return (dbconnection_t*)connection;
    }

    log_error("Postgresql error: %s\n", PQerrorMessage(connection->connection));

    postgresql_connection_free((dbconnection_t*)connection);

    return NULL;
}

const char* postgresql_table_exist_sql(const char* table) {
    char tmp[512] = {0};

    sprintf(
        &tmp[0],
        "SELECT "
            "1 "
        "FROM "
            "information_schema.tables "
        "WHERE "
            "table_name = '%s' AND "
            "table_type = 'BASE TABLE'",
        table
    );

    return strdup(&tmp[0]);
}

const char* postgresql_table_migration_create_sql()
{
    char tmp[512] = {0};

    strcpy(
        &tmp[0],
        "CREATE TABLE migration "
        "("
            "version     varchar(180)  NOT NULL PRIMARY KEY,"
            "apply_time  integer       NOT NULL DEFAULT 0"
        ")"
    );

    return strdup(&tmp[0]);
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

size_t postgresql_connection_string(char* buffer, size_t size, postgresqlhost_t* host) {
    return snprintf(buffer, size,
        "host=%s "
        "port=%d "
        "dbname=%s "
        "user=%s "
        "password=%s "
        "connect_timeout=%d ",
        host->base.ip,
        host->base.port,
        host->dbname,
        host->user,
        host->password,
        host->connection_timeout
    );
}

PGconn* postgresql_connect(dbhosts_t* hosts, int with_loop) {
    postgresqlhost_t* host = (postgresqlhost_t*)hosts->current_host;

    size_t string_length = postgresql_connection_string(NULL, 0, host);
    char* string = malloc(string_length + 1);
    if (string == NULL) return NULL;

    postgresql_connection_string(string, string_length, host);

    if (with_loop) db_next_host(hosts);

    PGconn* connection = PQconnectdb(string);

    free(string);

    return connection;
}

db_t* postgresql_load(const char* database_id, const jsontok_t* token_array) {
    db_t* result = NULL;
    db_t* database = db_create(database_id);
    if (database == NULL) goto failed;

    database->hosts = postgresql_hosts_create();
    if (database->hosts == NULL) goto failed;

    enum fields { PORT = 0, IP, DBNAME, USER, PASSWORD, CONNECTION_TIMEOUT, MIGRATION, FIELDS_COUNT };
    enum reqired_fields { R_PORT = 0, R_IP, R_DBNAME, R_USER, R_PASSWORD, R_CONNECTION_TIMEOUT, R_FIELDS_COUNT };
    int finded_fields[FIELDS_COUNT] = {0};
    dbhost_t* host_last = NULL;

    for (jsonit_t it_array = json_init_it(token_array); !json_end_it(&it_array); json_next_it(&it_array)) {
        jsontok_t* token_object = json_it_value(&it_array);
        postgresqlhost_t* host = postgresql_host_create();
        
        for (jsonit_t it_object = json_init_it(token_object); !json_end_it(&it_object); json_next_it(&it_object)) {
            const char* key = json_it_key(&it_object);
            jsontok_t* token_value = json_it_value(&it_object);

            if (strcmp(key, "port") == 0) {
                if (!json_is_int(token_value)) goto failed;

                finded_fields[PORT] = 1;

                host->base.port = json_int(token_value);
            }
            else if (strcmp(key, "ip") == 0) {
                if (!json_is_string(token_value)) goto failed;

                finded_fields[IP] = 1;

                const char* value = json_string(token_value);

                host->base.ip = (char*)malloc(strlen(value) + 1);
                if (host->base.ip == NULL) goto failed;

                strcpy(host->base.ip, value);
            }
            else if (strcmp(key, "dbname") == 0) {
                if (!json_is_string(token_value)) goto failed;

                finded_fields[DBNAME] = 1;

                const char* value = json_string(token_value);

                host->dbname = (char*)malloc(strlen(value) + 1);
                if (host->dbname == NULL) goto failed;

                strcpy(host->dbname, value);
            }
            else if (strcmp(key, "user") == 0) {
                if (!json_is_string(token_value)) goto failed;

                finded_fields[USER] = 1;

                const char* value = json_string(token_value);

                host->user = (char*)malloc(strlen(value) + 1);
                if (host->user == NULL) goto failed;

                strcpy(host->user, value);
            }
            else if (strcmp(key, "password") == 0) {
                if (!json_is_string(token_value)) goto failed;

                finded_fields[PASSWORD] = 1;

                const char* value = json_string(token_value);

                host->password = (char*)malloc(strlen(value) + 1);
                if (host->password == NULL) goto failed;

                strcpy(host->password, value);
            }
            else if (strcmp(key, "connection_timeout") == 0) {
                if (!json_is_int(token_value)) goto failed;

                finded_fields[CONNECTION_TIMEOUT] = 1;

                host->connection_timeout = json_int(token_value);
            }
            else if (strcmp(key, "migration") == 0) {
                if (!json_is_bool(token_value)) goto failed;

                finded_fields[MIGRATION] = 1;

                host->base.migration = json_bool(token_value);
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

        for (int i = 0; i < R_FIELDS_COUNT; i++) {
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
