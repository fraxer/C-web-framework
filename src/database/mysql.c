#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "../log/log.h"
#include "dbresult.h"
#include "mysql.h"

#define WITH_LOOP 1
#define WITHOUT_LOOP 0

dbhosts_t* my_hosts_create();
void my_connection_free(dbconnection_t*);
void my_send_query(dbresult_t*, dbconnection_t*, const char*);
MYSQL* my_connect(dbhosts_t*, int);

dbhosts_t* my_hosts_create() {
    dbhosts_t* hosts = malloc(sizeof *hosts);

    hosts->host = NULL;
    hosts->current_host = NULL;
    hosts->connection_create = my_connection_create;
    hosts->connection_create_manual = my_connection_create_manual;
    hosts->table_exist_sql = my_table_exist_sql;
    hosts->table_migration_create_sql = my_table_migration_create_sql;

    return hosts;
}

myhost_t* my_host_create() {
    myhost_t* host = malloc(sizeof *host);

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
    db_free(db);
}

dbconnection_t* my_connection_create(dbhosts_t* hosts) {
    myconnection_t* connection = (myconnection_t*)malloc(sizeof(myconnection_t));

    if (connection == NULL) return NULL;

    connection->base.locked = 0;
    connection->base.next = NULL;
    connection->base.free = my_connection_free;
    connection->base.send_query = my_send_query;

    void* host_address = hosts->current_host;

    while (1) {
        connection->connection = my_connect(hosts, WITH_LOOP);

        if (connection->connection != NULL) break;

        log_error("Mysql error: connection error\n");

        if (host_address == hosts->current_host) {
            my_connection_free((dbconnection_t*)connection);
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
    connection->base.free = my_connection_free;
    connection->base.send_query = my_send_query;
    connection->connection = my_connect(hosts, WITHOUT_LOOP);

    if (connection->connection != NULL) {
        return (dbconnection_t*)connection;
    }

    log_error("Mysql error: connection error\n");

    my_connection_free((dbconnection_t*)connection);

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

const char* my_table_migration_create_sql()
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

void my_connection_free(dbconnection_t* connection) {
    if (connection == NULL) return;

    myconnection_t* conn = (myconnection_t*)connection;

    mysql_close(conn->connection);
    free(conn);
}

void my_send_query(dbresult_t* result, dbconnection_t* connection, const char* string) {
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

MYSQL* my_connect(dbhosts_t* hosts, int with_loop) {
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
    if (database == NULL) goto failed;

    database->hosts = my_hosts_create();
    if (database->hosts == NULL) goto failed;

    enum fields { PORT = 0, IP, DBNAME, USER, PASSWORD, MIGRATION, FIELDS_COUNT };
    enum required_fields { R_PORT = 0, R_IP, R_DBNAME, R_USER, R_PASSWORD, R_FIELDS_COUNT };
    int finded_fields[FIELDS_COUNT] = {0};
    dbhost_t* host_last = NULL;

    for (jsonit_t it_array = json_init_it(token_array); !json_end_it(&it_array); json_next_it(&it_array)) {
        jsontok_t* token_object = json_it_value(&it_array);
        myhost_t* host = my_host_create();

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
                log_error("Error: Fill mysql config\n");
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
