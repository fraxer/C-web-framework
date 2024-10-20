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
#include "redis.h"

static redishost_t* __host_create(void);
static void __host_free(void* arg);
static void* __connection_create(void* host);
static void __connection_free(void* connection);
static dbresult_t* __query(void* connection, const char* sql);
int redis_send_command(redisContext*, const char*);
int redis_auth(redisContext*, const char*, const char*);
int redis_selectdb(redisContext*, const int);
static redisContext* __connect(void* host);
static int __is_active(void* connection);
static int __reconnect(void* host, void* connection);
static str_t* __escape_identifier(void* connection, str_t* str);
static str_t* __escape_string(void* connection, str_t* str);
static char* __compile_insert(void* connection, const char* table, array_t* params);
static char* __compile_select(void* connection, const char* table, array_t* columns, array_t* where);
static char* __compile_update(void* connection, const char* table, array_t* set, array_t* where);
static char* __compile_delete(void* connection, const char* table, array_t* where);


redishost_t* __host_create(void) {
    redishost_t* host = malloc(sizeof * host);
    if (host == NULL) return NULL;

    host->base.free = __host_free;
    host->base.port = 0;
    host->base.ip = NULL;
    host->base.id = NULL;
    host->base.connection_create = __connection_create;
    host->base.connections = array_create();
    host->base.connections_locked = 0;
    host->base.grammar.compile_table_exist = NULL;
    host->base.grammar.compile_table_migration_create = NULL;
    host->base.grammar.compile_insert = __compile_insert;
    host->base.grammar.compile_select = __compile_select;
    host->base.grammar.compile_update = __compile_update;
    host->base.grammar.compile_delete = __compile_delete;
    host->dbindex = 0;
    host->user = NULL;
    host->password = NULL;

    return host;
}

void __host_free(void* arg) {
    if (arg == NULL) return;

    redishost_t* host = arg;

    if (host->base.id) free(host->base.id);
    if (host->base.ip) free(host->base.ip);
    if (host->user) free(host->user);
    if (host->password) free(host->password);

    array_free(host->base.connections);
    free(host);
}

void* __connection_create(void* host) {
    redisconnection_t* connection = malloc(sizeof * connection);
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

void __connection_free(void* connection) {
    if (connection == NULL) return;

    redisconnection_t* conn = connection;

    redisFree(conn->connection);
    free(conn);
}

dbresult_t* __query(void* connection, const char* sql) {
    redisconnection_t* redisconnection = connection;

    dbresult_t* result = dbresult_create();
    if (result == NULL) return NULL;

    redisReply* reply = redisCommand(redisconnection->connection, sql);

    if (reply == NULL || redisconnection->connection->err != 0) {
        log_error("Redis error: %s\n", redisconnection->connection->errstr);
        goto failed;
    }

    if (reply->type == REDIS_REPLY_ERROR) {
        log_error("Redis error: %s\n", reply->str);
        goto failed;
    }

    int rows = reply->type == REDIS_REPLY_ARRAY ? reply->elements : 1;
    int cols = 1;
    int col = 0;
    dbresultquery_t* query = dbresult_query_create(rows, cols);

    if (query == NULL) {
        log_error("Out of memory\n");
        goto failed;
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

        dbresult_query_value_insert(query, value, length, row, col);
    }

    result->ok = 1;

    failed:

    freeReplyObject(reply);

    return result;
}

int redis_send_command(redisContext* connection, const char* string) {
    redisReply* reply = redisCommand(connection, string);
    if (reply == NULL) return 0;

    if (reply->type == REDIS_REPLY_ERROR)
        log_error("Redis error: %s\n", reply->str);

    freeReplyObject(reply);

    return 1;
}

int redis_auth(redisContext* connection, const char* user, const char* password) {
    const size_t string_length = 256;
    char string[string_length];

    const size_t user_length = strlen(user);
    const size_t password_length = strlen(password);

    if (string_length <= user_length + password_length + 6) {
        log_error("Redis error: user or password is too large");
        return 0;
    }

    const char* arg1 = user;
    const char* arg2 = password;

    if (user_length == 0) {
        arg1 = password;
        arg2 = user;
    }

    if (password_length == 0) return 1;

    sprintf(&string[0], "AUTH %s %s", arg1, arg2);

    return redis_send_command(connection, &string[0]);
}

int redis_selectdb(redisContext* connection, const int index) {
    char string[10];
    sprintf(&string[0], "SELECT %d", index);

    return redis_send_command(connection, &string[0]);
}

redisContext* __connect(void* arg) {
    redishost_t* host = arg;

    redisContext* connection = redisConnect(host->base.ip, host->base.port);
    if (connection == NULL || connection->err != 0) {
        log_error("Redis error: %s\n", connection->errstr);
        redisFree(connection);
        return NULL;
    }

    if (!redis_auth(connection, host->user, host->password)) {
        log_error("Redis error: %s\n", connection->errstr);
        redisFree(connection);
        return NULL;
    }

    if (!redis_selectdb(connection, host->dbindex)) {
        log_error("Redis error: %s\n", connection->errstr);
        redisFree(connection);
        return NULL;
    }

    redisEnableKeepAlive(connection);

    return connection;
}

int __is_active(void* connection) {
    redisconnection_t* conn = connection;
    if (conn == NULL) return 0;

    redisReply* reply = redisCommand(conn->connection, "PING");
    if (reply == NULL) {
        printf("Redis error: connection lost\n");
        return 0;
    }

    int is_alive = 0;
    if (reply->type == REDIS_REPLY_STRING && strcmp(reply->str, "PONG") == 0)
        is_alive = 1;
    else if (reply->type == REDIS_REPLY_STATUS && strcmp(reply->str, "PONG") == 0)
        is_alive = 1;

    freeReplyObject(reply);

    return is_alive;
}

int __reconnect(void* host, void* connection) {
    redisconnection_t* conn = connection;

    if (__is_active(conn->connection)) {
        redisFree(conn->connection);

        conn->connection = __connect(host);

        if (!__is_active(conn->connection)) {
            redisFree(conn->connection);
            conn->connection = NULL;
            return 0;
        }
    }

    return 1;
}

str_t* __escape_identifier(void* connection, str_t* str) {
    return __escape_string(connection, str);
}

str_t* __escape_string(void* connection, str_t* str) {
    (void)connection;
    str_t* quoted_str = str_create_empty();
    if (quoted_str == NULL) return NULL;

    str_appendc(quoted_str, '"');
    for (size_t i = 0; i < str_size(str); i++) {
        char ch = str_get(str)[i];
        if (ch == '"' || ch == '\\' || ch == '\'')
            str_appendc(quoted_str, '\\');

        str_appendc(quoted_str, ch);
    }
    str_appendc(quoted_str, '"');

    return quoted_str;
}

db_t* redis_load(const char* database_id, const jsontok_t* token_array) {
    db_t* result = NULL;
    db_t* database = db_create(database_id);
    if (database == NULL) return NULL;

    enum fields { HOST_ID = 0, PORT, IP, DBINDEX, USER, PASSWORD, FIELDS_COUNT };
    enum required_fields { R_HOST_ID = 0, R_PORT, R_IP, R_DBINDEX, R_FIELDS_COUNT };
    char* field_names[FIELDS_COUNT] = {"host_id", "port", "ip", "dbindex", "user", "password"};

    for (jsonit_t it_array = json_init_it(token_array); !json_end_it(&it_array); json_next_it(&it_array)) {
        jsontok_t* token_object = json_it_value(&it_array);
        int lresult = 0;
        int finded_fields[FIELDS_COUNT] = {0};
        redishost_t* host = __host_create();
        if (host == NULL) {
            log_error("redis_load: can't create host\n");
            goto failed;
        }

        for (jsonit_t it_object = json_init_it(token_object); !json_end_it(&it_object); json_next_it(&it_object)) {
            const char* key = json_it_key(&it_object);
            jsontok_t* token_value = json_it_value(&it_object);

            if (strcmp(key, "host_id") == 0) {
                if (finded_fields[HOST_ID]) {
                    log_error("redis_load: field %s must be unique\n", key);
                    goto host_failed;
                }
                if (!json_is_string(token_value)) {
                    log_error("redis_load: field %s must be string\n", key);
                    goto host_failed;
                }

                finded_fields[HOST_ID] = 1;

                if (host->base.id != NULL) free(host->base.id);

                host->base.id = malloc(token_value->size + 1);
                if (host->base.id == NULL) {
                    log_error("redis_load: alloc memory for %s failed\n", key);
                    goto host_failed;
                }

                strcpy(host->base.id, json_string(token_value));
            }
            else if (strcmp(key, "port") == 0) {
                if (finded_fields[PORT]) {
                    log_error("redis_load: field %s must be unique\n", key);
                    goto host_failed;
                }
                if (!json_is_int(token_value)) {
                    log_error("redis_load: field %s must be int\n", key);
                    goto host_failed;
                }

                finded_fields[PORT] = 1;

                host->base.port = json_int(token_value);
            }
            else if (strcmp(key, "ip") == 0) {
                if (finded_fields[IP]) {
                    log_error("redis_load: field %s must be unique\n", key);
                    goto host_failed;
                }
                if (!json_is_string(token_value)) {
                    log_error("redis_load: field %s must be string\n", key);
                    goto host_failed;
                }

                finded_fields[IP] = 1;

                if (host->base.ip != NULL) free(host->base.ip);

                host->base.ip = malloc(token_value->size + 1);
                if (host->base.ip == NULL) {
                    log_error("redis_load: alloc memory for %s failed\n", key);
                    goto host_failed;
                }

                strcpy(host->base.ip, json_string(token_value));
            }
            else if (strcmp(key, "dbindex") == 0) {
                if (finded_fields[DBINDEX]) {
                    log_error("redis_load: field %s must be unique\n", key);
                    goto host_failed;
                }
                if (!json_is_int(token_value)) {
                    log_error("redis_load: field %s must be int\n", key);
                    goto host_failed;
                }

                finded_fields[DBINDEX] = 1;

                host->dbindex = json_int(token_value);
                if (host->dbindex < 0 || host->dbindex > 16) {
                    log_error("redis_load: dbindex must be in range 0..16\n");
                    goto host_failed;
                }
            }
            else if (strcmp(key, "user") == 0) {
                if (finded_fields[USER]) {
                    log_error("redis_load: field %s must be unique\n", key);
                    goto host_failed;
                }
                if (!json_is_string(token_value)) {
                    log_error("redis_load: field %s must be string\n", key);
                    goto host_failed;
                }

                finded_fields[USER] = 1;

                if (host->user != NULL) free(host->user);

                host->user = malloc(token_value->size + 1);
                if (host->user == NULL) {
                    log_error("redis_load: alloc memory for %s failed\n", key);
                    goto host_failed;
                }

                strcpy(host->user, json_string(token_value));
            }
            else if (strcmp(key, "password") == 0) {
                if (finded_fields[PASSWORD]) {
                    log_error("redis_load: field %s must be unique\n", key);
                    goto host_failed;
                }
                if (!json_is_string(token_value)) {
                    log_error("redis_load: field %s must be string\n", key);
                    goto host_failed;
                }

                finded_fields[PASSWORD] = 1;

                if (host->password != NULL) free(host->password);

                host->password = malloc(token_value->size + 1);
                if (host->password == NULL) {
                    log_error("redis_load: alloc memory for %s failed\n", key);
                    goto host_failed;
                }

                strcpy(host->password, json_string(token_value));
            }
            else {
                log_error("redis_load: unknown field: %s\n", key);
                goto host_failed;
            }
        }

        array_push_back(database->hosts, array_create_pointer(host, array_nocopy, host->base.free));

        if (finded_fields[USER] == 0) {
            if (host->user == NULL)
                host->user = malloc(1);

            if (host->user == NULL) {
                log_error("redir_load: can't alloc memory for user\n");
                goto host_failed;
            }
            strcpy(host->user, "");
        }

        if (finded_fields[PASSWORD] == 0) {
            if (host->password == NULL)
                host->password = malloc(1);

            if (host->password == NULL) {
                log_error("redir_load: can't alloc memory for password\n");
                goto host_failed;
            }
            strcpy(host->password, "");
        }

        for (int i = 0; i < R_FIELDS_COUNT; i++) {
            if (finded_fields[i] == 0) {
                log_error("redir_load: required field %s not found\n", field_names[i]);
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
    (void)connection;
    (void)table;
    (void)params;
    return NULL;
}

char* __compile_select(void* connection, const char* table, array_t* columns, array_t* where) {
    (void)connection;
    (void)table;
    (void)columns;
    (void)where;
    return NULL;
}

char* __compile_update(void* connection, const char* table, array_t* set, array_t* where) {
    (void)connection;
    (void)table;
    (void)set;
    (void)where;
    return NULL;
}

char* __compile_delete(void* connection, const char* table, array_t* where) {
    (void)connection;
    (void)table;
    (void)where;
    return NULL;
}
