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
#include "mysql.h"

static myhost_t* __host_create(void);
static void __host_free(void*);
static void* __connection_create(void* host);
static void __connection_free(void* connection);
static dbresult_t* __query(void* connection, const char* sql);
static MYSQL* __connect(void* host);
static int __is_active(void* connection);
static int __reconnect(void* host, void* connection);
static char* __compile_table_exist(const char* table);
static char* __compile_table_migration_create(const char* table);
static str_t* __escape_identifier(void* connection, str_t* str);
static str_t* __escape_string(void* connection, str_t* str);
static str_t* __escape_internal(void* connection, str_t* str, char quote);
static char* __compile_insert(void* connection, const char* table, array_t* params);
static char* __compile_select(void* connection, const char* table, array_t* columns, array_t* where);
static char* __compile_update(void* connection, const char* table, array_t* set, array_t* where);
static char* __compile_delete(void* connection, const char* table, array_t* where);

myhost_t* __host_create(void) {
    myhost_t* host = malloc(sizeof * host);
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

    return host;
}

void __host_free(void* arg) {
    if (arg == NULL) return;

    myhost_t* host = arg;

    if (host->base.id) free(host->base.id);
    if (host->base.ip) free(host->base.ip);
    if (host->dbname) free(host->dbname);
    if (host->user) free(host->user);
    if (host->password) free(host->password);

    array_free(host->base.connections);
    free(host);
}

void* __connection_create(void* host) {
    myconnection_t* connection = malloc(sizeof * connection);
    if (connection == NULL) return NULL;

    connection->base.thread_id = gettid();
    connection->base.free = __connection_free;
    connection->base.query = __query;
    connection->base.escape_identifier = __escape_identifier;
    connection->base.escape_string = __escape_string;
    connection->base.is_active = __is_active;
    connection->base.reconnect = __reconnect;
    connection->connection = __connect(host);

    if (connection->connection == NULL) {
        connection->base.free(connection);
        connection = NULL;
    }

    return connection;
}

char* __compile_table_exist(const char* table) {
    char tmp[512] = {0};

    sprintf(
        &tmp[0],
        "SHOW TABLES LIKE '%s'",
        table
    );

    return strdup(&tmp[0]);
}

char* __compile_table_migration_create(const char* table) {
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

    myconnection_t* conn = connection;

    mysql_close(conn->connection);
    free(conn);
}

dbresult_t* __query(void* connection, const char* sql) {
    myconnection_t* myconnection = connection;

    dbresult_t* result = dbresult_create();
    if (result == NULL) return NULL;

    if (mysql_query(myconnection->connection, sql) != 0) {
        log_error("Mysql error: %s\n", mysql_error(myconnection->connection));
        return result;
    }

    result->ok = 1;

    int status = 0;
    dbresultquery_t* query_last = NULL;

    do {
        MYSQL_RES* res = NULL;

        if ((res = mysql_store_result(myconnection->connection))) {
            int cols = mysql_num_fields(res);
            int rows = mysql_num_rows(res);

            dbresultquery_t* query = dbresult_query_create(rows, cols);
            if (query == NULL) {
                result->ok = 0;
                log_error("Mysql error: Out of memory\n");
                goto clear;
            }

            if (query_last != NULL)
                query_last->next = query;

            query_last = query;

            if (result->query == NULL) {
                result->query = query;
                result->current = query;
            }

            MYSQL_FIELD* fields = mysql_fetch_fields(res);

            for (int col = 0; col < cols; col++)
                dbresult_query_field_insert(query, fields[col].name, col);

            MYSQL_ROW myrow;
            int row = 0;

            while ((myrow = mysql_fetch_row(res))) {
                unsigned long* lengths = mysql_fetch_lengths(res);

                for (int col = 0; col < cols; col++) {
                    size_t length = lengths[col];
                    const char* value = myrow[col];

                    dbresult_query_value_insert(query, value, length, row, col);
                }

                row++;
            }

            clear:

            mysql_free_result(res);
        }
        else if (mysql_field_count(myconnection->connection) != 0) {
            log_error("Mysql error: %s\n", mysql_error(myconnection->connection));
            result->ok = 0;
            break;
        }

        if ((status = mysql_next_result(myconnection->connection)) > 0) {
            log_error("Mysql error: %s\n", mysql_error(myconnection->connection));
            result->ok = 0;
        }

    } while (status == 0);

    return result;
}

MYSQL* __connect(void* arg) {
    myhost_t* host = arg;

    MYSQL* connection = mysql_init(NULL);
    if (connection == NULL) return NULL;

    int reconnect = 0;
    if (mysql_options(connection, MYSQL_OPT_RECONNECT, &reconnect)) {
        log_error("Failed to set reconnect option\n");
        mysql_close(connection);
        return NULL;
    }

    connection = mysql_real_connect(
        connection,
        host->base.ip,
        host->user,
        host->password,
        host->dbname,
        host->base.port,
        NULL,
        CLIENT_MULTI_STATEMENTS
    );

    return connection;
}

int __is_active(void* connection) {
    myconnection_t* conn = connection;
    if (conn == NULL) return 0;

    return mysql_ping(conn->connection) == 0;
}

int __reconnect(void* host, void* connection) {
    myconnection_t* conn = connection;

    if (__is_active(conn->connection)) {
        mysql_close(conn->connection);

        conn->connection = __connect(host);

        if (!__is_active(conn->connection)) {
            mysql_close(conn->connection);
            conn->connection = NULL;
            return 0;
        }
    }

    return 1;
}

str_t* __escape_identifier(void* connection, str_t* str) {
    return __escape_internal(connection, str, '"');
}

str_t* __escape_string(void* connection, str_t* str) {
    return __escape_internal(connection, str, '\'');
}

str_t* __escape_internal(void* connection, str_t* str, char quote) {
    myconnection_t* conn = connection;

    char* escaped = malloc(str_size(str) * 2 + 1);
    if (escaped == NULL) return NULL;

    unsigned long len = mysql_real_escape_string(conn->connection, escaped, str_get(str), str_size(str));
    if (len == (unsigned long)-1) {
        free(escaped);
        return NULL;
    }

    str_t* string = str_create_empty();
    if (string == NULL) {
        free(escaped);
        return NULL;
    }

    str_appendc(string, quote);
    str_append(string, escaped, len);
    str_appendc(string, quote);

    free(escaped);

    return string;
}

db_t* my_load(const char* database_id, const jsontok_t* token_array) {
    db_t* result = NULL;
    db_t* database = db_create(database_id);
    if (database == NULL) {
        log_error("my_load: can't create database\n");
        return NULL;
    }

    enum fields { HOST_ID = 0, PORT, IP, DBNAME, USER, PASSWORD, MIGRATION, FIELDS_COUNT };
    enum required_fields { R_HOST_ID = 0, R_PORT, R_IP, R_DBNAME, R_USER, R_PASSWORD, R_FIELDS_COUNT };
    char* field_names[FIELDS_COUNT] = {"host_id", "port", "ip", "dbname", "user", "password"};

    for (jsonit_t it_array = json_init_it(token_array); !json_end_it(&it_array); json_next_it(&it_array)) {
        jsontok_t* token_object = json_it_value(&it_array);
        int lresult = 0;
        int finded_fields[FIELDS_COUNT] = {0};
        myhost_t* host = __host_create();
        if (host == NULL) {
            log_error("my_load: can't create host\n");
            goto failed;
        }

        for (jsonit_t it_object = json_init_it(token_object); !json_end_it(&it_object); json_next_it(&it_object)) {
            const char* key = json_it_key(&it_object);
            jsontok_t* token_value = json_it_value(&it_object);

            if (strcmp(key, "host_id") == 0) {
                if (finded_fields[HOST_ID]) {
                    log_error("my_load: field %s must be unique\n", key);
                    goto host_failed;
                }
                if (!json_is_string(token_value)) {
                    log_error("my_load: field %s must be string\n", key);
                    goto host_failed;
                }

                finded_fields[HOST_ID] = 1;

                if (host->base.id != NULL) free(host->base.id);

                host->base.id = malloc(token_value->size + 1);
                if (host->base.id == NULL) {
                    log_error("my_load: alloc memory for %s failed\n", key);
                    goto host_failed;
                }

                strcpy(host->base.id, json_string(token_value));
            }
            else if (strcmp(key, "port") == 0) {
                if (finded_fields[PORT]) {
                    log_error("my_load: field %s must be unique\n", key);
                    goto host_failed;
                }
                if (!json_is_int(token_value)) {
                    log_error("my_load: field %s must be int\n", key);
                    goto host_failed;
                }

                finded_fields[PORT] = 1;

                host->base.port = json_int(token_value);
            }
            else if (strcmp(key, "ip") == 0) {
                if (finded_fields[IP]) {
                    log_error("my_load: field %s must be unique\n", key);
                    goto host_failed;
                }
                if (!json_is_string(token_value)) {
                    log_error("my_load: field %s must be string\n", key);
                    goto host_failed;
                }

                finded_fields[IP] = 1;

                if (host->base.ip != NULL) free(host->base.ip);

                host->base.ip = malloc(token_value->size + 1);
                if (host->base.ip == NULL) {
                    log_error("my_load: alloc memory for %s failed\n", key);
                    goto host_failed;
                }

                strcpy(host->base.ip, json_string(token_value));
            }
            else if (strcmp(key, "dbname") == 0) {
                if (finded_fields[DBNAME]) {
                    log_error("my_load: field %s must be unique\n", key);
                    goto host_failed;
                }
                if (!json_is_string(token_value)) {
                    log_error("my_load: field %s must be string\n", key);
                    goto host_failed;
                }

                finded_fields[DBNAME] = 1;

                if (host->dbname != NULL) free(host->dbname);

                host->dbname = malloc(token_value->size + 1);
                if (host->dbname == NULL) {
                    log_error("my_load: alloc memory for %s failed\n", key);
                    goto host_failed;
                }

                strcpy(host->dbname, json_string(token_value));
            }
            else if (strcmp(key, "user") == 0) {
                if (finded_fields[USER]) {
                    log_error("my_load: field %s must be unique\n", key);
                    goto host_failed;
                }
                if (!json_is_string(token_value)) {
                    log_error("my_load: field %s must be string\n", key);
                    goto host_failed;
                }

                finded_fields[USER] = 1;

                if (host->user != NULL) free(host->user);

                host->user = malloc(token_value->size + 1);
                if (host->user == NULL) {
                    log_error("my_load: alloc memory for %s failed\n", key);
                    goto host_failed;
                }

                strcpy(host->user, json_string(token_value));
            }
            else if (strcmp(key, "password") == 0) {
                if (finded_fields[PASSWORD]) {
                    log_error("my_load: field %s must be unique\n", key);
                    goto host_failed;
                }
                if (!json_is_string(token_value)) {
                    log_error("my_load: field %s must be string\n", key);
                    goto host_failed;
                }

                finded_fields[PASSWORD] = 1;

                if (host->password != NULL) free(host->password);

                host->password = malloc(token_value->size + 1);
                if (host->password == NULL) {
                    log_error("my_load: alloc memory for %s failed\n", key);
                    goto host_failed;
                }

                strcpy(host->password, json_string(token_value));
            }
            else {
                log_error("my_load: unknown field: %s\n", key);
                goto host_failed;
            }
        }

        array_push_back(database->hosts, array_create_pointer(host, array_nocopy, host->base.free));

        for (int i = 0; i < R_FIELDS_COUNT; i++) {
            if (finded_fields[i] == 0) {
                log_error("my_load: required field %s not found\n", field_names[i]);
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

    if (result == NULL)
        db_free(database);

    return result;
}

char* __compile_insert(void* connection, const char* table, array_t* params) {
    if (connection == NULL) return NULL;
    if (table == NULL) return NULL;
    if (params == NULL) return NULL;
    if (array_size(params) == 0) return NULL;

    char* buffer = NULL;

    str_t* fields = str_create_empty();
    if (fields == NULL) return NULL;

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

        str_t* quoted_str = __escape_string(connection, value);
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

char* __compile_select(void* connection, const char* table, array_t* columns, array_t* where) {
    if (connection == NULL) return 0;
    if (table == NULL) return 0;

    char* buffer = NULL;

    str_t* columns_str = str_create_empty();
    if (columns_str == NULL) return 0;

    str_t* where_str = str_create_empty();
    if (where_str == NULL) goto failed;

    for (size_t i = 0; i < array_size(columns); i++) {
        const char* column_name = array_get(columns, i);

        if (i > 0)
            str_appendc(columns_str, ',');

        str_append(columns_str, column_name, strlen(column_name));
    }

    for (size_t i = 0; i < array_size(where); i++) {
        mfield_t* field = array_get(where, i);

        if (i > 0)
            str_append(where_str, " AND ", 5);

        str_append(where_str, field->name, strlen(field->name));
        str_appendc(where_str, '=');

        str_t* value = model_field_to_string(field);
        if (value == NULL) goto failed;

        str_t* quoted_str = __escape_string(connection, value);
        if (quoted_str == NULL) goto failed;

        str_append(where_str, str_get(quoted_str), str_size(quoted_str));
        str_free(quoted_str);
    }

    const char* format = "SELECT %s FROM %s WHERE %s";
    const size_t buffer_size = strlen(format) + strlen(table) + str_size(columns_str) + str_size(where_str) + 1;
    buffer = malloc(buffer_size);
    if (buffer == NULL) goto failed;

    snprintf(buffer, buffer_size,
        format,
        str_get(columns_str),
        table,
        str_get(where_str)
    );

    failed:

    str_free(columns_str);
    str_free(where_str);

    return buffer;
}

char* __compile_update(void* connection, const char* table, array_t* set, array_t* where) {
    if (connection == NULL) return 0;
    if (table == NULL) return 0;
    if (set == NULL) return 0;

    char* buffer = NULL;

    str_t* set_str = str_create_empty();
    if (set_str == NULL) return 0;

    str_t* where_str = str_create_empty();
    if (where_str == NULL) goto failed;

    if (where == NULL || array_size(where) == 0)
        str_append(where_str, "true", 4);

    for (size_t i = 0; i < array_size(set); i++) {
        mfield_t* field = array_get(set, i);

        if (i > 0)
            str_appendc(set_str, ',');

        str_append(set_str, field->name, strlen(field->name));
        str_appendc(set_str, '=');

        str_t* value = model_field_to_string(field);
        if (value == NULL) goto failed;

        str_t* quoted_str = __escape_string(connection, value);
        if (quoted_str == NULL) goto failed;

        str_append(set_str, str_get(quoted_str), str_size(quoted_str));
        str_free(quoted_str);
    }

    for (size_t i = 0; i < array_size(where); i++) {
        mfield_t* field = array_get(where, i);

        if (i > 0)
            str_append(where_str, " AND ", 5);

        str_append(where_str, field->name, strlen(field->name));
        str_appendc(where_str, '=');

        str_t* value = model_field_to_string(field);
        if (value == NULL) goto failed;

        str_t* quoted_str = __escape_string(connection, value);
        if (quoted_str == NULL) goto failed;

        str_append(where_str, str_get(quoted_str), str_size(quoted_str));
        str_free(quoted_str);
    }

    const char* format = "UPDATE %s SET %s WHERE %s";
    const size_t buffer_size = strlen(format) + strlen(table) + str_size(set_str) + str_size(where_str) + 1;
    buffer = malloc(buffer_size);
    if (buffer == NULL) goto failed;

    snprintf(buffer, buffer_size,
        format,
        table,
        str_get(set_str),
        str_get(where_str)
    );

    failed:

    str_free(set_str);
    str_free(where_str);

    return buffer;
}

char* __compile_delete(void* connection, const char* table, array_t* where) {
    if (connection == NULL) return 0;
    if (table == NULL) return 0;

    char* buffer = NULL;

    str_t* where_str = str_create_empty();
    if (where_str == NULL) goto failed;

    if (where == NULL || array_size(where) == 0)
        str_append(where_str, "true", 4);

    for (size_t i = 0; i < array_size(where); i++) {
        mfield_t* field = array_get(where, i);

        if (i > 0)
            str_append(where_str, " AND ", 5);

        str_append(where_str, field->name, strlen(field->name));
        str_appendc(where_str, '=');

        str_t* value = model_field_to_string(field);
        if (value == NULL) goto failed;

        str_t* quoted_str = __escape_string(connection, value);
        if (quoted_str == NULL) goto failed;

        str_append(where_str, str_get(quoted_str), str_size(quoted_str));
        str_free(quoted_str);
    }

    const char* format = "DELETE FROM %s WHERE %s";
    const size_t buffer_size = strlen(format) + strlen(table) + str_size(where_str) + 1;
    buffer = malloc(buffer_size);
    if (buffer == NULL) goto failed;

    snprintf(buffer, buffer_size,
        format,
        table,
        str_get(where_str)
    );

    failed:

    str_free(where_str);

    return buffer;
}
