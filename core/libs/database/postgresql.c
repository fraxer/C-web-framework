#define _GNU_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include "log.h"
#include "array.h"
#include "str.h"
#include "model.h"
#include "dbresult.h"
#include "postgresql.h"

static postgresqlhost_t* __host_create(void);
static void __host_free(void*);
static void* __connection_create(void* host);
static void __connection_free(void* connection);
static dbresult_t* __query(void* connection, const char* sql);
static PGconn* __connect(void* host);
static char* __compile_table_exist(const char* table);
static char* __compile_table_migration_create(const char* table);
static str_t* __qoute_string(void* connection, str_t* str);
static char* __compile_insert(void* connection, const char* table, array_t* params);
static char* __compile_select(void* connection, const char* table, array_t* columns, array_t* params);
static char* __compile_update(void* connection, const char* table, array_t* set, array_t* params);
static char* __compile_delete(void* connection, const char* table, array_t* params);

postgresqlhost_t* __host_create(void) {
    postgresqlhost_t* host = malloc(sizeof * host);
    if (host == NULL) return NULL;

    host->base.free = __host_free;
    host->base.port = 0;
    host->base.ip = NULL;
    host->base.id = NULL;
    host->base.connection_create = __connection_create;
    host->base.connections = array_create();
    host->base.connections_locked = 0;
    host->base.grammar.compile_table_exist = __compile_table_exist;
    host->base.grammar.compile_table_migration_create = __compile_table_migration_create;
    host->base.grammar.compile_insert = __compile_insert;
    host->base.grammar.compile_select = __compile_select;
    host->base.grammar.compile_update = __compile_update;
    host->base.grammar.compile_delete = __compile_delete;
    host->dbname = NULL;
    host->user = NULL;
    host->password = NULL;
    host->connection_timeout = 0;

    return host;
}

void __host_free(void* arg) {
    if (arg == NULL) return;

    postgresqlhost_t* host = arg;

    if (host->base.ip) free(host->base.ip);
    if (host->dbname) free(host->dbname);
    if (host->user) free(host->user);
    if (host->password) free(host->password);
    host->base.port = 0;
    host->connection_timeout = 0;

    free(host);
}

void* __connection_create(void* host) {
    postgresqlconnection_t* connection = malloc(sizeof * connection);
    if (connection == NULL) return NULL;

    connection->base.thread_id = gettid();
    connection->base.free = __connection_free;
    connection->base.query = __query;
    connection->base.quote_string = __qoute_string;
    connection->connection = __connect(host);

    if (PQstatus(connection->connection) != CONNECTION_OK) {
        log_error("Postgresql error: %s\n", PQerrorMessage(connection->connection));
        __connection_free((dbconnection_t*)connection);
        return NULL;
    }

    return connection;
}

char* __compile_table_exist(const char* table) {
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

char* __compile_table_migration_create(const char* table)
{
    char tmp[512] = {0};

    sprintf(
        &tmp[0],
        "CREATE TABLE %s "
        "("
            "version     varchar(180)  NOT NULL PRIMARY KEY,"
            "apply_time  integer       NOT NULL DEFAULT 0"
        ")",
        table
    );

    return strdup(&tmp[0]);
}

void __connection_free(void* connection) {
    if (connection == NULL) return;

    postgresqlconnection_t* conn = connection;

    PQfinish(conn->connection);
    free(conn);
}

dbresult_t* __query(void* connection, const char* sql) {
    postgresqlconnection_t* pgconnection = (postgresqlconnection_t*)connection;
    dbresult_t* result = dbresult_create();

    if (!PQsendQuery(pgconnection->connection, sql)) {
        log_error("Postgresql error: %s", PQerrorMessage(pgconnection->connection));
        strncpy(result->error, PQerrorMessage(pgconnection->connection), sizeof result->error);
        return result;
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
                strncpy(result->error, "Out of memory", sizeof result->error);
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
            strncpy(result->error, PQresultErrorMessage(res), sizeof result->error);
        }
        else if (status == PGRES_NONFATAL_ERROR) {
            log_error("Postgresql Nonfatal error: %s", PQresultErrorMessage(res));
            result->ok = 0;
            strncpy(result->error, PQresultErrorMessage(res), sizeof result->error);
        }
        else if (status == PGRES_EMPTY_QUERY) {
            log_error("Postgresql Empty query: %s", PQresultErrorMessage(res));
            result->ok = 0;
            strncpy(result->error, PQresultErrorMessage(res), sizeof result->error);
        }
        else if (status == PGRES_BAD_RESPONSE) {
            log_error("Postgresql Bad response: %s", PQresultErrorMessage(res));
            result->ok = 0;
            strncpy(result->error, PQresultErrorMessage(res), sizeof result->error);
        }

        clear:

        PQclear(res);
    }

    return result;
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

PGconn* __connect(void* arg) {
    postgresqlhost_t* host = arg;

    size_t string_length = postgresql_connection_string(NULL, 0, host);
    char* string = malloc(string_length + 1);
    if (string == NULL) return NULL;

    postgresql_connection_string(string, string_length, host);

    PGconn* connection = PQconnectdb(string);

    free(string);

    return connection;
}

str_t* __qoute_string(void* connection, str_t* str) {
    postgresqlconnection_t* conn = connection;

    char* quoted = malloc(2 * str_size(str) + 3);
    if (quoted == NULL) return NULL;

    int error = 0;
    const size_t quotedlen = PQescapeStringConn(conn->connection, quoted, str_get(str), str_size(str), &error);
    if (error) {
        free(quoted);
        return NULL;
    }

    str_t* quoted_str = str_create_empty();
    if (quoted_str == NULL) {
        free(quoted);
        return NULL;
    }

    str_appendc(quoted_str, '\'');
    str_append(quoted_str, quoted, quotedlen);
    str_appendc(quoted_str, '\'');

    free(quoted);

    return quoted_str;
}

db_t* postgresql_load(const char* database_id, const jsontok_t* token_array) {
    db_t* result = NULL;
    db_t* database = db_create(database_id);
    if (database == NULL) {
        log_error("postgresql_load: can't create database\n");
        return NULL;
    }

    enum fields { PORT = 0, IP, DBNAME, USER, PASSWORD, CONNECTION_TIMEOUT, FIELDS_COUNT };
    enum reqired_fields { R_PORT = 0, R_IP, R_DBNAME, R_USER, R_PASSWORD, R_CONNECTION_TIMEOUT, R_FIELDS_COUNT };
    char* field_names[FIELDS_COUNT] = {"port", "ip", "dbname", "user", "password", "connection_timeout"};

    for (jsonit_t it_array = json_init_it(token_array); !json_end_it(&it_array); json_next_it(&it_array)) {
        jsontok_t* token_object = json_it_value(&it_array);
        int lresult = 0;
        int finded_fields[FIELDS_COUNT] = {0};
        postgresqlhost_t* host = __host_create();
        if (host == NULL) {
            log_error("postgresql_load: can't create host\n");
            goto failed;
        }

        for (jsonit_t it_object = json_init_it(token_object); !json_end_it(&it_object); json_next_it(&it_object)) {
            const char* key = json_it_key(&it_object);
            jsontok_t* token_value = json_it_value(&it_object);

            if (strcmp(key, "port") == 0) {
                if (finded_fields[PORT]) {
                    log_error("postgresql_load: field %s must be unique\n", key);
                    goto host_failed;
                }
                if (!json_is_int(token_value)) {
                    log_error("postgresql_load: field %s must be int\n", key);
                    goto host_failed;
                }

                finded_fields[PORT] = 1;

                host->base.port = json_int(token_value);
            }
            else if (strcmp(key, "ip") == 0) {
                if (finded_fields[IP]) {
                    log_error("postgresql_load: field %s must be unique\n", key);
                    goto host_failed;
                }
                if (!json_is_string(token_value)) {
                    log_error("postgresql_load: field %s must be string\n", key);
                    goto host_failed;
                }

                finded_fields[IP] = 1;

                if (host->base.ip != NULL) free(host->base.ip);

                host->base.ip = malloc(token_value->size + 1);
                if (host->base.ip == NULL) {
                    log_error("postgresql_load: alloc memory for %s failed\n", key);
                    goto host_failed;
                }

                strcpy(host->base.ip, json_string(token_value));
            }
            else if (strcmp(key, "dbname") == 0) {
                if (finded_fields[DBNAME]) {
                    log_error("postgresql_load: field %s must be unique\n", key);
                    goto host_failed;
                }
                if (!json_is_string(token_value)) {
                    log_error("postgresql_load: field %s must be string\n", key);
                    goto host_failed;
                }

                finded_fields[DBNAME] = 1;

                if (host->dbname != NULL) free(host->dbname);

                host->dbname = malloc(token_value->size + 1);
                if (host->dbname == NULL) {
                    log_error("postgresql_load: alloc memory for %s failed\n", key);
                    goto host_failed;
                }

                strcpy(host->dbname, json_string(token_value));
            }
            else if (strcmp(key, "user") == 0) {
                if (finded_fields[USER]) {
                    log_error("postgresql_load: field %s must be unique\n", key);
                    goto host_failed;
                }
                if (!json_is_string(token_value)) {
                    log_error("postgresql_load: field %s must be string\n", key);
                    goto host_failed;
                }

                finded_fields[USER] = 1;

                if (host->user != NULL) free(host->user);

                host->user = malloc(token_value->size + 1);
                if (host->user == NULL) {
                    log_error("postgresql_load: alloc memory for %s failed\n", key);
                    goto host_failed;
                }

                strcpy(host->user, json_string(token_value));
            }
            else if (strcmp(key, "password") == 0) {
                if (finded_fields[PASSWORD]) {
                    log_error("postgresql_load: field %s must be unique\n", key);
                    goto host_failed;
                }
                if (!json_is_string(token_value)) {
                    log_error("postgresql_load: field %s must be string\n", key);
                    goto host_failed;
                }

                finded_fields[PASSWORD] = 1;

                if (host->password != NULL) free(host->password);

                host->password = malloc(token_value->size + 1);
                if (host->password == NULL) {
                    log_error("postgresql_load: alloc memory for %s failed\n", key);
                    goto host_failed;
                }

                strcpy(host->password, json_string(token_value));
            }
            else if (strcmp(key, "connection_timeout") == 0) {
                if (finded_fields[CONNECTION_TIMEOUT]) {
                    log_error("postgresql_load: field %s must be unique\n", key);
                    goto host_failed;
                }
                if (!json_is_int(token_value)) {
                    log_error("postgresql_load: field %s must be int\n", key);
                    goto host_failed;
                }

                finded_fields[CONNECTION_TIMEOUT] = 1;

                host->connection_timeout = json_int(token_value);
            }
            else {
                log_error("postgresql_load: unknown field: %s\n", key);
                goto host_failed;
            }
        }

        array_push_back(database->hosts, array_create_pointer(host, host->base.free));

        for (int i = 0; i < R_FIELDS_COUNT; i++) {
            if (finded_fields[i] == 0) {
                log_error("postgresql_load: required field %s not found\n", field_names[i]);
                goto host_failed;
            }
        }

        lresult = 1;

        host_failed:

        if (lresult == 0) {
            host->base.free(host);
            goto failed;
        }
    }

    result = database;

    failed:

    if (result == NULL) {
        db_free(database);
    }

    return result;
}

char* __compile_insert(void* connection, const char* table, array_t* params) {
    if (connection == NULL) return 0;
    if (table == NULL) return 0;
    if (params == NULL) return 0;

    char* buffer = NULL;

    str_t* fields = str_create_empty();
    if (fields == NULL) return 0;

    str_t* values = str_create_empty();
    if (values == NULL) goto failed;

    for (size_t i = 0; i < array_size(params); i++) {
        mfield_t* field = array_get(params, i);

        if (i > 0) {
            str_appendc(fields, ',');
            str_appendc(values, ',');
        }

        str_append(fields, field->name, strlen(field->name));

        str_t* value = model_field_to_string(field);
        if (value == NULL) goto failed;

        str_t* quoted_str = __qoute_string(connection, value);
        if (quoted_str == NULL) goto failed;

        str_append(values, str_get(quoted_str), str_size(quoted_str));
        str_free(quoted_str);
    }

    const char* format = "INSERT INTO %s (%s) VALUES (%s)";
    const size_t buffer_size = strlen(format) + strlen(table) + str_size(fields) + str_size(values) + 1;
    buffer = malloc(buffer_size);
    if (buffer == NULL) goto failed;

    snprintf(buffer, buffer_size,
        format,
        table,
        str_get(fields),
        str_get(values)
    );

    failed:

    str_free(fields);
    str_free(values);

    return buffer;
}

char* __compile_select(void* connection, const char* table, array_t* columns, array_t* params) {
    (void)columns;
    (void)params;
    if (connection == NULL) return 0;
    if (table == NULL) return 0;
    if (params == NULL) return 0;

    char* buffer = NULL;

    str_t* fields = str_create_empty();
    if (fields == NULL) return 0;

    str_t* values = str_create_empty();
    if (values == NULL) goto failed;

    for (size_t i = 0; i < array_size(params); i++) {
        mfield_t* field = array_get(params, i);

        if (i > 0) {
            str_appendc(fields, ',');
            str_appendc(values, ',');
        }

        str_append(fields, field->name, strlen(field->name));

        str_t* value = model_field_to_string(field);
        if (value == NULL) goto failed;

        str_t* quoted_str = __qoute_string(connection, value);
        if (quoted_str == NULL) goto failed;

        str_append(values, str_get(quoted_str), str_size(quoted_str));
        str_free(quoted_str);
    }

    const char* format = "INSERT INTO %s (%s) VALUES (%s)";
    const size_t buffer_size = strlen(format) + strlen(table) + str_size(fields) + str_size(values) + 1;
    buffer = malloc(buffer_size);
    if (buffer == NULL) goto failed;

    snprintf(buffer, buffer_size,
        format,
        table,
        str_get(fields),
        str_get(values)
    );

    failed:

    str_free(fields);
    str_free(values);

    return buffer;
}

char* __compile_update(void* connection, const char* table, array_t* set, array_t* params) {
    (void)set;
    (void)params;
    if (connection == NULL) return 0;
    if (table == NULL) return 0;
    if (params == NULL) return 0;

    char* buffer = NULL;

    str_t* fields = str_create_empty();
    if (fields == NULL) return 0;

    str_t* values = str_create_empty();
    if (values == NULL) goto failed;

    for (size_t i = 0; i < array_size(params); i++) {
        mfield_t* field = array_get(params, i);

        if (i > 0) {
            str_appendc(fields, ',');
            str_appendc(values, ',');
        }

        str_append(fields, field->name, strlen(field->name));

        str_t* value = model_field_to_string(field);
        if (value == NULL) goto failed;

        str_t* quoted_str = __qoute_string(connection, value);
        if (quoted_str == NULL) goto failed;

        str_append(values, str_get(quoted_str), str_size(quoted_str));
        str_free(quoted_str);
    }

    const char* format = "INSERT INTO %s (%s) VALUES (%s)";
    const size_t buffer_size = strlen(format) + strlen(table) + str_size(fields) + str_size(values) + 1;
    buffer = malloc(buffer_size);
    if (buffer == NULL) goto failed;

    snprintf(buffer, buffer_size,
        format,
        table,
        str_get(fields),
        str_get(values)
    );

    failed:

    str_free(fields);
    str_free(values);

    return buffer;
}

char* __compile_delete(void* connection, const char* table, array_t* params) {
    if (connection == NULL) return 0;
    if (table == NULL) return 0;
    if (params == NULL) return 0;

    char* buffer = NULL;

    str_t* fields = str_create_empty();
    if (fields == NULL) return 0;

    str_t* values = str_create_empty();
    if (values == NULL) goto failed;

    for (size_t i = 0; i < array_size(params); i++) {
        mfield_t* field = array_get(params, i);

        if (i > 0) {
            str_appendc(fields, ',');
            str_appendc(values, ',');
        }

        str_append(fields, field->name, strlen(field->name));

        str_t* value = model_field_to_string(field);
        if (value == NULL) goto failed;

        str_t* quoted_str = __qoute_string(connection, value);
        if (quoted_str == NULL) goto failed;

        str_append(values, str_get(quoted_str), str_size(quoted_str));
        str_free(quoted_str);
    }

    const char* format = "INSERT INTO %s (%s) VALUES (%s)";
    const size_t buffer_size = strlen(format) + strlen(table) + str_size(fields) + str_size(values) + 1;
    buffer = malloc(buffer_size);
    if (buffer == NULL) goto failed;

    snprintf(buffer, buffer_size,
        format,
        table,
        str_get(fields),
        str_get(values)
    );

    failed:

    str_free(fields);
    str_free(values);

    return buffer;
}
