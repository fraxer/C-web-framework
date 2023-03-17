#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "../log/log.h"
#include "dbresult.h"
#include "mysql.h"
    #include <stdio.h>

// static int my_lib_init = 0;

void my_connection_free(dbconnection_t*);
dbresult_t my_send_query(dbconnection_t*, const char*);
MYSQL* my_connect(myconfig_t*);

myconfig_t* my_config_create() {
    myconfig_t* config = (myconfig_t*)malloc(sizeof(myconfig_t));

    config->base.free = my_config_free;
    config->base.connection_create = my_connection_create;
    config->host = NULL;
    config->current_host = NULL;
    config->dbname = NULL;
    config->user = NULL;
    config->password = NULL;

    return config;
}

void my_config_free(void* arg) {
    if (arg == NULL) return;

    myconfig_t* config = (myconfig_t*)arg;

    db_host_free(config->host);
    config->host = NULL;

    if (config->dbname) free(config->dbname);
    if (config->user) free(config->user);
    if (config->password) free(config->password);

    free(config);
}

void my_free(db_t* db) {
    db_free(db);
}

dbconnection_t* my_connection_create(dbconfig_t* config) {
    myconfig_t* myconfig = (myconfig_t*)config;

    myconnection_t* connection = (myconnection_t*)malloc(sizeof(myconnection_t));

    if (connection == NULL) return NULL;

    connection->base.locked = 0;
    connection->base.next = NULL;
    connection->base.free = my_connection_free;
    connection->base.send_query = my_send_query;

    void* host_address = myconfig->current_host;

    while (1) {
        connection->connection = my_connect(myconfig);

        if (connection->connection != NULL) break;

        log_error("Mysql error: %s\n", mysql_error(connection->connection));

        if (host_address == myconfig->current_host) {
            my_connection_free((dbconnection_t*)connection);
            return NULL;
        }

        mysql_close(connection->connection);
    }

    return (dbconnection_t*)connection;
}

const char* my_host(myconfig_t* config) {
    return config->current_host->ip;
}

int my_port(myconfig_t* config) {
    return config->current_host->port;
}

void my_next_host(myconfig_t* config) {
    if (config->current_host->next != NULL) {
        config->current_host = config->current_host->next;
        return;
    }

    config->current_host = config->host;
}

void my_connection_free(dbconnection_t* connection) {
    if (connection == NULL) return;

    myconnection_t* conn = (myconnection_t*)connection;

    mysql_close(conn->connection);
    free(conn);
}

dbresult_t my_send_query(dbconnection_t* connection, const char* string) {
    myconnection_t* myconnection = (myconnection_t*)connection;

    dbresult_t result = {
        .ok = 0,
        .error_code = 0,
        .error_message = "",
        .query = NULL,
        .current = NULL
    };

    if (mysql_query(myconnection->connection, string) != 0) {
        log_error("Mysql error: %s\n", mysql_error(myconnection->connection));
        result.error_code = 1;
        result.error_message = "Mysql error: connection error";
        return result;
    }

    int status = 0;
    dbresultquery_t* query_last = NULL;

    do {
        MYSQL_RES* res = NULL;

        if (res = mysql_store_result(myconnection->connection)) {
            int cols = mysql_num_fields(res);
            int rows = mysql_num_rows(res);

            dbresultquery_t* query = dbresult_query_create(rows, cols);

            if (query == NULL) {
                result.error_message = "Out of memory";
                return result;
            }

            MYSQL_FIELD* fields = mysql_fetch_fields(res);

            for (int col = 0; col < cols; col++) {
                // printf("Field %u is %s\n", col, fields[col].name);

                dbresult_query_field_insert(query, fields[col].name, col);
            }

            MYSQL_ROW myrow;
            int row = 0;
            while ((myrow = mysql_fetch_row(res))) {
                unsigned long* lengths = mysql_fetch_lengths(res);

                for (int col = 0; col < cols; col++) {
                    size_t length = lengths[col];
                    const char* value = myrow[col];

                    db_table_cell_t* cell = dbresult_cell_create(value, length);

                    if (cell == NULL) {
                        result.error_message = "Out of memory";
                        return result;
                    }

                    dbresult_query_table_insert(query, cell, row, col);

                    // printf("[%.*s] ", (int) lengths[col], myrow[col] ? myrow[col] : "NULL");
               }

               row++;

               // printf("\n");
            }

            if (query_last != NULL) {
                query_last->next = query;
            }
            query_last = query;

            if (result.query == NULL) {
                result.query = query;
                result.current = query;
            }

            result.ok = 1;

            mysql_free_result(res);
        }
        else if (mysql_field_count(myconnection->connection) != 0) {
            log_error("Mysql error: %s\n", mysql_error(myconnection->connection));
            result.error_code = 1;
            result.error_message = mysql_error(myconnection->connection);
        }

        if ((status = mysql_next_result(myconnection->connection)) > 0) {
            log_error("Mysql error: %s\n", mysql_error(myconnection->connection));
            result.ok = 0;
            result.error_message = mysql_error(myconnection->connection);
        }

    } while (status == 0);

    return result;
}

MYSQL* my_connect(myconfig_t* config) {
    MYSQL* connection = mysql_init(NULL);

    if (connection == NULL) return NULL;

    connection = mysql_real_connect(
        connection,
        my_host(config),
        config->user,
        config->password,
        config->dbname,
        my_port(config),
        NULL,
        CLIENT_MULTI_STATEMENTS
    );

    my_next_host(config);

    return connection;
}

db_t* my_load(const char* database_id, const jsmntok_t* token_object) {
    db_t* result = NULL;
    db_t* database = db_create(database_id);

    if (database == NULL) goto failed;

    myconfig_t* config = my_config_create();

    if (config == NULL) goto failed;

    enum fields { HOSTS = 0, DBNAME, USER, PASSWORD, FIELDS_COUNT };

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
    }

    for (int i = 0; i < FIELDS_COUNT; i++) {
        if (finded_fields[i] == 0) {
            log_error("Error: Fill database config\n");
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
