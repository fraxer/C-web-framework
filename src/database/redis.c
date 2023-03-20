#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "../log/log.h"
#include "dbresult.h"
#include "redis.h"
    #include <stdio.h>

void redis_connection_free(dbconnection_t*);
dbresult_t redis_send_query(dbconnection_t*, const char*);
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

dbresult_t redis_send_query(dbconnection_t* connection, const char* string) {
    redisconnection_t* redisconnection = (redisconnection_t*)connection;

    dbresult_t result = {
        .ok = 0,
        .error_code = 0,
        .error_message = "",
        .query = NULL,
        .current = NULL
    };

    redisReply* reply = redisCommand(redisconnection->connection, string);

    if (reply == NULL || redisconnection->connection->err != 0) {
        log_error("Redis error: %s\n", redisconnection->connection->errstr);
        result.error_code = 1;
        result.error_message = "Redis error: connection error";
        freeReplyObject(reply);
        return result;
    }

    if (reply->type == REDIS_REPLY_ERROR) {
        log_error("Redis error: %s\n", reply->str);
        result.error_message = "Redis error: query error";
        freeReplyObject(reply);
        return result;
    }

    // printf("%ld %s\n", reply->len, reply->str);

    int rows = reply->type == REDIS_REPLY_ARRAY ? reply->elements : 1;
    int cols = 1;
    int col = 0;
    dbresultquery_t* query = dbresult_query_create(rows, cols);

    if (query == NULL) {
        result.error_message = "Out of memory";
        freeReplyObject(reply);
        return result;
    }

    result.query = query;
    result.current = query;

    dbresult_query_field_insert(query, "", col);
    
    for (int row = 0; row < rows; row++) {
        size_t length = reply->len;
        const char* value = reply->str;

        if (rows > 1) {
            length = reply->element[row]->len;
            value = reply->element[row]->str;
        }

        db_table_cell_t* cell = dbresult_cell_create(value, length);

        if (cell == NULL) {
            result.error_message = "Out of memory";
            freeReplyObject(reply);
            return result;
        }

        dbresult_query_table_insert(query, cell, row, col);
    }

    result.ok = 1;

    freeReplyObject(reply);

    return result;
}

redisContext* redis_connect(redisconfig_t* config) {
    redisContext* connection = redisConnect(redis_host(config), redis_port(config));

    if (connection == NULL || connection->err != 0) {
        log_error("Redis error: %s\n", connection->errstr);
        if (connection) redisFree(connection);
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
