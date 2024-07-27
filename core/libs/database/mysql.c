#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include "log.h"
#include "dbresult.h"
#include "mysql.h"

#define WITH_LOOP 1
#define WITHOUT_LOOP 0

static dbhosts_t* __my_hosts_create(void);
static void __my_connection_free(dbconnection_t* connection);
static void __my_send_query(dbresult_t* result, dbconnection_t* connection, const char* string);
static MYSQL* __my_connect(dbhosts_t* hosts, int with_loop);

dbhosts_t* __my_hosts_create(void) {
    dbhosts_t* hosts = malloc(sizeof * hosts);
    if (hosts == NULL) return NULL;

    hosts->host = NULL;
    hosts->current_host = NULL;
    hosts->connection_create = my_connection_create;
    hosts->connection_create_manual = my_connection_create_manual;
    hosts->table_exist_sql = my_table_exist_sql;
    hosts->table_migration_create_sql = my_table_migration_create_sql;

    return hosts;
}

myhost_t* my_host_create(void) {
    myhost_t* host = malloc(sizeof *host);
    if (host == NULL) return NULL;

    host->base.free = my_host_free;
    host->base.migration = 0;
    host->base.port = 0;
    host->base.ip = NULL;
    host->base.next = NULL;
    host->dbname = NULL;
    host->user = NULL;
    host->password = NULL;

    return host;
}

void my_host_free(void* arg) {
    if (arg == NULL) return;

    myhost_t* host = arg;

    if (host->base.ip) free(host->base.ip);
    if (host->dbname) free(host->dbname);
    if (host->user) free(host->user);
    if (host->password) free(host->password);
    host->base.port = 0;
    host->base.next = NULL;

    free(host);
}

void my_free(db_t* db) {
    db_destroy(db);
}

dbconnection_t* my_connection_create(dbhosts_t* hosts) {
    myconnection_t* connection = (myconnection_t*)malloc(sizeof(myconnection_t));

    if (connection == NULL) return NULL;

    connection->base.locked = 0;
    connection->base.next = NULL;
    connection->base.free = __my_connection_free;
    connection->base.send_query = __my_send_query;

    void* host_address = hosts->current_host;

    while (1) {
        connection->connection = __my_connect(hosts, WITH_LOOP);

        if (connection->connection != NULL) break;

        log_error("Mysql error: connection error\n");

        if (host_address == hosts->current_host) {
            __my_connection_free((dbconnection_t*)connection);
            return NULL;
        }

        mysql_close(connection->connection);
    }

    return (dbconnection_t*)connection;
}

dbconnection_t* my_connection_create_manual(dbhosts_t* hosts) {
    myconnection_t* connection = (myconnection_t*)malloc(sizeof(myconnection_t));

    if (connection == NULL) return NULL;

    connection->base.locked = 0;
    connection->base.next = NULL;
    connection->base.free = __my_connection_free;
    connection->base.send_query = __my_send_query;
    connection->connection = __my_connect(hosts, WITHOUT_LOOP);

    if (connection->connection != NULL) {
        return (dbconnection_t*)connection;
    }

    log_error("Mysql error: connection error\n");

    __my_connection_free((dbconnection_t*)connection);

    return NULL;
}

const char* my_table_exist_sql(const char* table) {
    char tmp[512] = {0};

    sprintf(
        &tmp[0],
        "SELECT "
            "1 "
        "FROM "
            "information_schema.TABLES "
        "WHERE "
            "TABLE_TYPE LIKE 'BASE TABLE' AND "
            "TABLE_NAME = '%s' ",
        table
    );

    return strdup(&tmp[0]);
}

const char* my_table_migration_create_sql(void) {
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

void __my_connection_free(dbconnection_t* connection) {
    if (connection == NULL) return;

    myconnection_t* conn = (myconnection_t*)connection;

    mysql_close(conn->connection);
    free(conn);
}

void __my_send_query(dbresult_t* result, dbconnection_t* connection, const char* string) {
    myconnection_t* myconnection = (myconnection_t*)connection;

    if (mysql_query(myconnection->connection, string) != 0) {
        log_error("Mysql error: %s\n", mysql_error(myconnection->connection));
        result->error_message = "Mysql error: connection error";
        return;
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
                result->error_message = "Out of memory";
                goto clear;
            }

            if (query_last != NULL) {
                query_last->next = query;
            }
            query_last = query;

            if (result->query == NULL) {
                result->query = query;
                result->current = query;
            }

            MYSQL_FIELD* fields = mysql_fetch_fields(res);

            for (int col = 0; col < cols; col++) {
                dbresult_query_field_insert(query, fields[col].name, col);
            }

            MYSQL_ROW myrow;
            int row = 0;

            while ((myrow = mysql_fetch_row(res))) {
                unsigned long* lengths = mysql_fetch_lengths(res);

                for (int col = 0; col < cols; col++) {
                    size_t length = lengths[col];
                    const char* value = myrow[col];

                    dbresult_query_table_insert(query, value, length, row, col);
               }

               row++;
            }

            clear:

            mysql_free_result(res);
        }
        else if (mysql_field_count(myconnection->connection) != 0) {
            log_error("Mysql error: %s\n", mysql_error(myconnection->connection));
            result->ok = 0;
            result->error_message = mysql_error(myconnection->connection);
            break;
        }

        if ((status = mysql_next_result(myconnection->connection)) > 0) {
            log_error("Mysql error: %s\n", mysql_error(myconnection->connection));
            result->ok = 0;
            result->error_message = mysql_error(myconnection->connection);
        }

    } while (status == 0);

    return;
}

MYSQL* __my_connect(dbhosts_t* hosts, int with_loop) {
    MYSQL* connection = mysql_init(NULL);

    if (connection == NULL) return NULL;

    myhost_t* host = (myhost_t*)hosts->current_host;

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

    if (with_loop) db_next_host(hosts);

    return connection;
}

db_t* my_load(const char* database_id, const jsontok_t* token_array) {
    db_t* result = NULL;
    db_t* database = db_create(database_id);
    if (database == NULL) return NULL;

    database->hosts = __my_hosts_create();
    if (database->hosts == NULL) {
        log_error("my_load: can't create hosts\n");
        goto failed;
    }

    enum fields { PORT = 0, IP, DBNAME, USER, PASSWORD, MIGRATION, FIELDS_COUNT };
    enum required_fields { R_PORT = 0, R_IP, R_DBNAME, R_USER, R_PASSWORD, R_FIELDS_COUNT };
    char* field_names[FIELDS_COUNT] = {"port", "ip", "dbname", "user", "password", "migration"};
    dbhost_t* host_last = NULL;

    for (jsonit_t it_array = json_init_it(token_array); !json_end_it(&it_array); json_next_it(&it_array)) {
        jsontok_t* token_object = json_it_value(&it_array);
        int lresult = 0;
        int finded_fields[FIELDS_COUNT] = {0};
        myhost_t* host = my_host_create();
        if (host == NULL) {
            log_error("my_load: can't create host\n");
            goto failed;
        }

        for (jsonit_t it_object = json_init_it(token_object); !json_end_it(&it_object); json_next_it(&it_object)) {
            const char* key = json_it_key(&it_object);
            jsontok_t* token_value = json_it_value(&it_object);

            if (strcmp(key, "port") == 0) {
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
                if (host->password == NULL) goto host_failed;

                strcpy(host->password, json_string(token_value));
            }
            else if (strcmp(key, "migration") == 0) {
                if (finded_fields[MIGRATION]) {
                    log_error("my_load: field %s must be unique\n", key);
                    goto host_failed;
                }
                if (!json_is_bool(token_value)) {
                    log_error("my_load: field %s must be bool\n", key);
                    goto host_failed;
                }

                finded_fields[MIGRATION] = 1;

                host->base.migration = json_bool(token_value);
            }
            else {
                log_error("my_load: unknown field: %s\n", key);
                goto host_failed;
            }
        }

        if (database->hosts->host == NULL) {
            database->hosts->host = (dbhost_t*)host;
            database->hosts->current_host = (dbhost_t*)host;
        }
        if (host_last != NULL)
            host_last->next = (dbhost_t*)host;

        host_last = (dbhost_t*)host;

        for (int i = 0; i < R_FIELDS_COUNT; i++) {
            if (finded_fields[i] == 0) {
                log_error("my_load: required field %s not found\n", field_names[i]);
                goto host_failed;
            }
        }

        lresult = 1;

        host_failed:

        if (lresult == 0) {
            my_host_free(host);
            goto failed;
        }
    }

    result = database;

    failed:

    if (result == NULL)
        db_destroy(database);

    return result;
}
