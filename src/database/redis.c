#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "../log/log.h"
#include "dbresult.h"
#include "redis.h"

void redis_connection_free(dbconnection_t*);
void redis_send_query(dbresult_t*, dbconnection_t*, const char*);
int redis_send_command(redisContext*, const char*);
int redis_auth(redisContext*, const char*, const char*);
int redis_selectdb(redisContext*, const int);
redisContext* redis_connect(redisconfig_t*);

redisconfig_t* redis_config_create() {
    redisconfig_t* config = (redisconfig_t*)malloc(sizeof(redisconfig_t));

    config->base.free = redis_config_free;
    config->base.connection_create = redis_connection_create;
    config->host = NULL;
    config->current_host = NULL;
    config->dbindex = 0;
    config->user = NULL;
    config->password = NULL;

    return config;
}

void redis_config_free(void* arg) {
    if (arg == NULL) return;

    redisconfig_t* config = (redisconfig_t*)arg;

    db_host_free(config->host);
    config->host = NULL;

    config->dbindex = 0;

    if (config->user) free(config->user);
    if (config->password) free(config->password);

    free(config);
}

void redis_free(db_t* db) {
    db_free(db);
}

dbconnection_t* redis_connection_create(dbconfig_t* config) {
    redisconfig_t* redisconfig = (redisconfig_t*)config;

    redisconnection_t* connection = (redisconnection_t*)malloc(sizeof(redisconnection_t));

    if (connection == NULL) return NULL;

    connection->base.locked = 0;
    connection->base.next = NULL;
    connection->base.free = redis_connection_free;
    connection->base.send_query = redis_send_query;

    void* host_address = redisconfig->current_host;

    while (1) {
        connection->connection = redis_connect(redisconfig);

        if (connection->connection != NULL) break;

        log_error("Redis error: connection error\n");

        if (host_address == redisconfig->current_host) {
            redis_connection_free((dbconnection_t*)connection);
            return NULL;
        }

        redisFree(connection->connection);
    }

    return (dbconnection_t*)connection;
}

const char* redis_host(redisconfig_t* config) {
    return config->current_host->ip;
}

int redis_port(redisconfig_t* config) {
    return config->current_host->port;
}

void redis_next_host(redisconfig_t* config) {
    if (config->current_host->next != NULL) {
        config->current_host = config->current_host->next;
        return;
    }

    config->current_host = config->host;
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

redisContext* redis_connect(redisconfig_t* config) {
    redisContext* connection = redisConnect(redis_host(config), redis_port(config));

    if (connection == NULL || connection->err != 0) {
        log_error("Redis error: %s\n", connection->errstr);
        if (connection) redisFree(connection);
        return NULL;
    }

    if (redis_auth(connection, config->user, config->password) == -1) {
        redisFree(connection);
        return NULL;
    }

    if (redis_selectdb(connection, config->dbindex) == -1) {
        redisFree(connection);
        return NULL;
    }

    redisEnableKeepAlive(connection);

    redis_next_host(config);

    return connection;
}

db_t* redis_load(const char* database_id, const jsmntok_t* token_object) {
    db_t* result = NULL;
    db_t* database = db_create(database_id);

    if (database == NULL) goto failed;

    redisconfig_t* config = redis_config_create();

    if (config == NULL) goto failed;

    enum fields { HOSTS = 0, DBINDEX, USER, PASSWORD, FIELDS_COUNT };
    enum required_fields { R_HOSTS = 0, R_DBINDEX, R_FIELDS_COUNT };

    int finded_fields[FIELDS_COUNT] = {0};

    for (jsmntok_t* token = token_object->child; token; token = token->sibling) {
        const char* key = jsmn_get_value(token);

        if (strcmp(key, "hosts") == 0) {
            finded_fields[HOSTS] = 1;

            config->host = db_hosts_load(token->child);
            config->current_host = config->host;

            if (config->host == NULL) goto failed;
        }
        else if (strcmp(key, "dbindex") == 0) {
            finded_fields[DBINDEX] = 1;

            const char* value = jsmn_get_value(token->child);

            config->dbindex = atoi(value);

            if (config->dbindex < 0 || config->dbindex > 16) goto failed;
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

    if (finded_fields[USER] == 0) {
        config->user = (char*)malloc(1);
        if (config->user == NULL) goto failed;
        strcpy(config->user, "");
    }

    if (finded_fields[PASSWORD] == 0) {
        config->password = (char*)malloc(1);
        if (config->password == NULL) goto failed;
        strcpy(config->password, "");
    }

    for (int i = 0; i < R_FIELDS_COUNT; i++) {
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
