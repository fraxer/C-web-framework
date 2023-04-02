#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "../log/log.h"
#include "dbresult.h"
#include "mysql.h"

void my_connection_free(dbconnection_t*);
void my_send_query(dbresult_t*, dbconnection_t*, const char*);
MYSQL* my_connect(dbhosts_t*);

myhost_t* my_host_create() {
    myhost_t* host = malloc(sizeof *host);

    host->base.free = my_host_free;
    host->base.next = NULL;
    host->port = 0;
    host->ip = NULL;
    host->dbname = NULL;
    host->user = NULL;
    host->password = NULL;

    return host;
}

void my_host_free(void* arg) {
    if (arg == NULL) return;

    myhost_t* host = arg;

    if (host->ip) free(host->ip);
    if (host->dbname) free(host->dbname);
    if (host->user) free(host->user);
    if (host->password) free(host->password);
    host->port = 0;
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
        connection->connection = my_connect(hosts);

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

void my_next_host(dbhosts_t* hosts) {
    if (hosts->current_host->next != NULL) {
        hosts->current_host = hosts->current_host->next;
        return;
    }

    hosts->current_host = hosts->host;
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

MYSQL* my_connect(dbhosts_t* hosts) {
    MYSQL* connection = mysql_init(NULL);

    if (connection == NULL) return NULL;

    myhost_t* host = (myhost_t*)hosts->current_host;

    connection = mysql_real_connect(
        connection,
        host->ip,
        host->user,
        host->password,
        host->dbname,
        host->port,
        NULL,
        CLIENT_MULTI_STATEMENTS
    );

    my_next_host(hosts);

    return connection;
}

db_t* my_load(const char* database_id, const jsmntok_t* token_array) {
    db_t* result = NULL;
    db_t* database = db_create(database_id);
    if (database == NULL) goto failed;

    database->hosts = db_hosts_create(my_connection_create);
    if (database->hosts == NULL) goto failed;

    enum fields { PORT = 0, IP, DBNAME, USER, PASSWORD, FIELDS_COUNT };
    int finded_fields[FIELDS_COUNT] = {0};
    dbhost_t* host_last = NULL;

    for (jsmntok_t* token_object = token_array->child; token_object; token_object = token_object->sibling) {
        myhost_t* host = my_host_create();

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
