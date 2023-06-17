#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "../log/log.h"
#include "dbresult.h"
#include "redis.h"

dbhosts_t* redis_hosts_create();
void redis_connection_free(dbconnection_t*);
void redis_send_query(dbresult_t*, dbconnection_t*, const char*);
int redis_send_command(redisContext*, const char*);
int redis_auth(redisContext*, const char*, const char*);
int redis_selectdb(redisContext*, const int);
redisContext* redis_connect(dbhosts_t*);

dbhosts_t* redis_hosts_create() {
    dbhosts_t* hosts = malloc(sizeof *hosts);

    hosts->host = NULL;
    hosts->current_host = NULL;
    hosts->connection_create = redis_connection_create;
    hosts->connection_create_manual = NULL;
    hosts->table_exist_sql = NULL;
    hosts->table_migration_create_sql = NULL;

    return hosts;
}

redishost_t* redis_host_create() {
    redishost_t* host = malloc(sizeof *host);

    host->base.free = redis_host_free;
    host->base.migration = 0;
    host->base.port = 0;
    host->base.ip = NULL;
    host->base.next = NULL;
    host->dbindex = 0;
    host->user = NULL;
    host->password = NULL;

    return host;
}

void redis_host_free(void* arg) {
    if (arg == NULL) return;

    redishost_t* host = arg;

    if (host->base.ip) free(host->base.ip);
    if (host->user) free(host->user);
    if (host->password) free(host->password);
    host->base.port = 0;
    host->dbindex = 0;
    host->base.next = NULL;

    free(host);
}

void redis_free(db_t* db) {
    db_free(db);
}

dbconnection_t* redis_connection_create(dbhosts_t* hosts) {
    redisconnection_t* connection = (redisconnection_t*)malloc(sizeof(redisconnection_t));

    if (connection == NULL) return NULL;

    connection->base.locked = 0;
    connection->base.next = NULL;
    connection->base.free = redis_connection_free;
    connection->base.send_query = redis_send_query;

    void* host_address = hosts->current_host;

    while (1) {
        connection->connection = redis_connect(hosts);

        if (connection->connection != NULL) break;

        log_error("Redis error: connection error\n");

        if (host_address == hosts->current_host) {
            redis_connection_free((dbconnection_t*)connection);
            return NULL;
        }

        redisFree(connection->connection);
    }

    return (dbconnection_t*)connection;
}

void redis_connection_free(dbconnection_t* connection) {
    if (connection == NULL) return;

    redisconnection_t* conn = (redisconnection_t*)connection;

    redisFree(conn->connection);
    free(conn);
}

void redis_send_query(dbresult_t* result, dbconnection_t* connection, const char* string) {
    redisconnection_t* redisconnection = (redisconnection_t*)connection;

    redisReply* reply = redisCommand(redisconnection->connection, string);

    if (reply == NULL || redisconnection->connection->err != 0) {
        log_error("Redis error: %s\n", redisconnection->connection->errstr);
        result->error_message = "Redis error: connection error";
        freeReplyObject(reply);
        return;
    }

    if (reply->type == REDIS_REPLY_ERROR) {
        log_error("Redis error: %s\n", reply->str);
        result->error_message = "Redis error: query error";
        freeReplyObject(reply);
        return;
    }

    int rows = reply->type == REDIS_REPLY_ARRAY ? reply->elements : 1;
    int cols = 1;
    int col = 0;
    dbresultquery_t* query = dbresult_query_create(rows, cols);

    if (query == NULL) {
        result->error_message = "Out of memory";
        freeReplyObject(reply);
        return;
    }

    result->query = query;
    result->current = query;

    dbresult_query_field_insert(query, "", col);

    for (int row = 0; row < rows; row++) {
        size_t length = reply->len;
        const char* value = reply->str;

        if (rows > 1) {
            length = reply->element[row]->len;
            value = reply->element[row]->str;
        }

        dbresult_query_table_insert(query, value, length, row, col);
    }

    result->ok = 1;

    freeReplyObject(reply);

    return;
}

int redis_send_command(redisContext* connection, const char* string) {
    int result = -1;

    redisReply* reply = redisCommand(connection, string);

    if (reply == NULL) return result;

    if (reply->type == REDIS_REPLY_ERROR)
        log_error("Redis error: %s\n", reply->str);
    else
        result = 0;

    freeReplyObject(reply);

    return 0;
}

int redis_auth(redisContext* connection, const char* user, const char* password) {
    size_t string_length = 256;
    char string[string_length];

    size_t user_length = strlen(user);
    size_t password_length = strlen(password);

    if (string_length <= user_length + password_length + 6) {
        log_error("Redis error: user or password is too large");
        return -1;
    }

    const char* arg1 = user;
    const char* arg2 = password;

    if (user_length == 0) {
        arg1 = password;
        arg2 = user;
    }

    if (password_length == 0) return 0;

    sprintf(&string[0], "AUTH %s %s", arg1, arg2);

    return redis_send_command(connection, &string[0]);
}

int redis_selectdb(redisContext* connection, const int index) {
    char string[10];
    sprintf(&string[0], "SELECT %d", index);

    return redis_send_command(connection, &string[0]);
}

redisContext* redis_connect(dbhosts_t* hosts) {
    redishost_t* host = (redishost_t*)hosts->current_host;
    redisContext* connection = redisConnect(host->base.ip, host->base.port);

    if (connection == NULL || connection->err != 0) {
        log_error("Redis error: %s\n", connection->errstr);
        if (connection) redisFree(connection);
        return NULL;
    }

    if (redis_auth(connection, host->user, host->password) == -1) {
        redisFree(connection);
        return NULL;
    }

    if (redis_selectdb(connection, host->dbindex) == -1) {
        redisFree(connection);
        return NULL;
    }

    redisEnableKeepAlive(connection);

    db_next_host(hosts);

    return connection;
}

db_t* redis_load(const char* database_id, const jsontok_t* token_array) {
    db_t* result = NULL;
    db_t* database = db_create(database_id);
    if (database == NULL) goto failed;

    database->hosts = redis_hosts_create();
    if (database->hosts == NULL) goto failed;

    enum fields { PORT = 0, IP, DBINDEX, USER, PASSWORD, FIELDS_COUNT };
    enum required_fields { R_PORT = 0, R_IP, R_DBINDEX, R_FIELDS_COUNT };
    int finded_fields[FIELDS_COUNT] = {0};
    dbhost_t* host_last = NULL;

    for (jsonit_t it_array = json_init_it(token_array); !json_end_it(&it_array); json_next_it(&it_array)) {
        jsontok_t* token_object = json_it_value(&it_array);
        redishost_t* host = redis_host_create();

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
            else if (strcmp(key, "dbindex") == 0) {
                if (!json_is_int(token_value)) goto failed;

                finded_fields[DBINDEX] = 1;

                host->dbindex = json_int(token_value);

                if (host->dbindex < 0 || host->dbindex > 16) goto failed;
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
        }

        if (database->hosts->host == NULL) {
            database->hosts->host = (dbhost_t*)host;
            database->hosts->current_host = (dbhost_t*)host;
        }
        if (host_last != NULL) {
            host_last->next = (dbhost_t*)host;
        }
        host_last = (dbhost_t*)host;

        if (finded_fields[USER] == 0) {
            host->user = (char*)malloc(1);
            if (host->user == NULL) goto failed;
            strcpy(host->user, "");
        }

        if (finded_fields[PASSWORD] == 0) {
            host->password = (char*)malloc(1);
            if (host->password == NULL) goto failed;
            strcpy(host->password, "");
        }

        for (int i = 0; i < R_FIELDS_COUNT; i++) {
            if (finded_fields[i] == 0) {
                log_error("Error: Fill database config\n");
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
