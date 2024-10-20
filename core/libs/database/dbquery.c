#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>

#include "appconfig.h"
#include "str.h"
#include "log.h"
#include "dbquery.h"
#include "model.h"

static dbhost_t* __get_host(const char* identificator);
static int __ispunct(int c);
static str_t* __build_query(dbconnection_t* connection, const char* query, array_t* params);
static int __process_value(dbconnection_t* connection, char parameter_type, str_t* string, str_t* value);

dbinstance_t* dbinstance(const char* identificator) {
    dbhost_t* host = __get_host(identificator);
    if (host == NULL) {
        log_error("db host not found\n");
        return NULL;
    }

    dbinstance_t* inst = malloc(sizeof * inst);
    if (inst == NULL) return NULL;

    host_connections_lock(host);
    dbconnection_t* connection = db_connection_find(host->connections);
    host_connections_unlock(host);

    int result = 0;
    if (connection == NULL) {
        connection = host->connection_create(host);

        if (connection == NULL) {
            log_error("db connection not found\n");
            goto exit;
        }

        host_connections_lock(host);
        array_push_back(host->connections, array_create_pointer(connection, array_nocopy, connection->free));
        host_connections_unlock(host);
    }

    if (!connection->is_active(connection)) {
        if (!connection->reconnect(host, connection)) {
            log_error("db reconnect error\n");
            goto exit;
        }
    }

    inst->grammar = &host->grammar;
    inst->connection = connection;

    result = 1;

    exit:

    if (result == 0) {
        dbinstance_free(inst);
        inst = NULL;
    }

    return inst;
}

void dbinstance_free(dbinstance_t* instance) {
    free(instance);
}

dbresult_t* dbqueryf(dbinstance_t* instance, const char* format, ...) {
    va_list args;
    va_start(args, format);
    size_t string_length = vsnprintf(NULL, 0, format, args);
    va_end(args);

    char* string = malloc(string_length + 1);
    if (string == NULL) return NULL;

    va_start(args, format);
    vsnprintf(string, string_length + 1, format, args);
    va_end(args);

    dbconnection_t* connection = instance->connection;
    dbresult_t* result = connection->query(connection, string);

    free(string);

    return result;
}

dbresult_t* dbquery(dbinstance_t* instance, const char* format, array_t* params) {
    dbconnection_t* connection = instance->connection;

    str_t* result_query = __build_query(connection, format, params);
    if (result_query == NULL) return NULL;

    printf("%s\n", str_get(result_query));

    dbresult_t* result = connection->query(connection, str_get(result_query));

    str_free(result_query);

    return result;
}

dbresult_t* dbtable_exist(dbinstance_t* instance, const char* table) {
    char* sql = instance->grammar->compile_table_exist(table);
    if (sql == NULL) return NULL;

    dbconnection_t* connection = instance->connection;
    dbresult_t* result = connection->query(connection, sql);

    free(sql);

    return result;
}

dbresult_t* dbtable_migration_create(dbinstance_t* instance, const char* table) {
    char* sql = instance->grammar->compile_table_migration_create(table);
    if (sql == NULL) return NULL;

    dbconnection_t* connection = instance->connection;
    dbresult_t* result = connection->query(connection, sql);

    free(sql);

    return result;
}

dbresult_t* dbbegin(dbinstance_t* instance, transaction_level_e level) {
    (void)level;
    return dbqueryf(instance, "level");
}

dbresult_t* dbcommit(dbinstance_t* instance) {
    return dbqueryf(instance, "commit");
}

dbresult_t* dbrollback(dbinstance_t* instance) {
    return dbqueryf(instance, "rollback");
}

dbresult_t* dbinsert(dbinstance_t* instance, const char* table, array_t* params) {
    dbconnection_t* connection = instance->connection;

    char* sql = instance->grammar->compile_insert(connection, table, params);
    if (sql == NULL) return NULL;

    dbresult_t* result = connection->query(connection, sql);

    free(sql);

    return result;
}

dbresult_t* dbupdate(dbinstance_t* instance, const char* table, array_t* set, array_t* where) {
    dbconnection_t* connection = instance->connection;

    char* sql = instance->grammar->compile_update(connection, table, set, where);
    if (sql == NULL) return NULL;

    dbresult_t* result = connection->query(connection, sql);

    free(sql);

    return result;
}

dbresult_t* dbdelete(dbinstance_t* instance, const char* table, array_t* where) {
    dbconnection_t* connection = instance->connection;

    char* sql = instance->grammar->compile_delete(connection, table, where);
    if (sql == NULL) return NULL;

    dbresult_t* result = connection->query(connection, sql);

    free(sql);

    return result;
}

dbresult_t* dbselect(dbinstance_t* instance, const char* table, array_t* columns, array_t* where) {
    dbconnection_t* connection = instance->connection;

    char* sql = instance->grammar->compile_select(connection, table, columns, where);
    if (sql == NULL) return NULL;

    dbresult_t* result = connection->query(connection, sql);

    free(sql);

    return result;
}

dbhost_t* __get_host(const char* identificator) {
    if (identificator == NULL) return NULL;

    const char* p = identificator;
    while (*p != '.' && *p != 0) p++;

    if (p - identificator == 0) return NULL;

    char* driver = strndup(identificator, p - identificator);
    if (driver == NULL) return NULL;

    const char* host_id = NULL;
    if (*p != 0 && strlen(p + 1) > 0)
        host_id = p + 1;

    dbhost_t* host = NULL;
    array_t* dbs = appconfig()->databases;
    for (size_t i = 0; i < array_size(dbs); i++) {
        db_t* db = array_get(dbs, i);
        if (strcmp(db->id, driver) == 0) {
            host = db_host_find(db, host_id);
            break;
        }
    }

    free(driver);

    return host;
}

int __ispunct(int c) {
    if (c >= 32 && c <= 126) {
        if ((c >= '0' && c <= '9') ||
            (c >= 'A' && c <= 'Z') ||
            (c >= 'a' && c <= 'z') ||
            (c == ' ')) {
            return 0;
        }

        if (c == '_') return 0;

        return 1;
    }

    return 0;
}

str_t* __build_query(dbconnection_t* connection, const char* query, array_t* params) {
    #define MAX_PARAM_NAME 256
    const size_t query_size = strlen(query);
    int param_start = -1;
    char param_name[MAX_PARAM_NAME] = {0};
    str_t* result_query = str_create_empty();
    if (result_query == NULL) return NULL;

    typedef struct {
        size_t position;
        size_t offset;
    } point_t;

    point_t point = {0, 0};
    int result = 0;
    char parameter_type = ':';

    for (size_t i = 0; i < query_size; i++) {
        if (query[i] == ':' || query[i] == '@') {
            if (param_start != -1) {
                printf("error param concats\n");
                goto failed;
            }

            param_start = i;
            point.offset = i;
            parameter_type = query[i];
            continue;
        }

        const int string_end = i == query_size - 1;
        const int is_spec_symbol = __ispunct(query[i]) || iscntrl(query[i]) || isspace(query[i]);
        if (param_start != -1 && (is_spec_symbol || string_end)) {
            const size_t param_end = (string_end && !is_spec_symbol) ? i + 1 : i;

            size_t name_size = param_end - param_start - 1;
            if (name_size > 0 && name_size < MAX_PARAM_NAME) {
                strncpy(param_name, query + param_start + 1, MAX_PARAM_NAME - 1);
                param_name[name_size] = 0;
                int param_finded = 0;
                int is_in = 0;
                const char* param_name_p = param_name;

                if (starts_with_substr(param_name, "list__")) {
                    param_name_p += 6;
                    is_in = 1;
                }

                for (size_t j = 0; j < array_size(params); j++) {
                    mfield_t* field = array_get(params, j);
                    if (strcmp(param_name_p, field->name) == 0) {
                        str_append(result_query, query + point.position, point.offset - point.position);

                        if (is_in) {
                            if (field->type != MODEL_ARRAY) {
                                printf("dbquery error: param not array <%s>\n", param_name_p);
                                goto failed;
                            }

                            array_t* array = model_array(field);
                            const size_t size = array_size(array);
                            if (size == 0) goto failed;

                            for (size_t k = 0; k < size; k++) {
                                str_t* str = array_item_to_string(array, k);
                                if (str == NULL) goto failed;
                                if (k > 0)
                                    str_appendc(result_query, ',');

                                if (!__process_value(connection, parameter_type, result_query, str))
                                    goto failed;

                                str_free(str);
                            }
                        }
                        else {
                            str_t* field_value = model_field_to_string(field);

                            if (!__process_value(connection, parameter_type, result_query, field_value))
                                goto failed;
                        }

                        point.position = param_end;
                        point.offset = param_end;

                        param_finded = 1;
                        break;
                    }
                }

                if (!param_finded) {
                    printf("error param not found in params array <%s>\n", param_name_p);
                    goto failed;
                }
            }
            param_start = -1;
        }

        if (param_start == -1 && string_end)
            str_append(result_query, query + point.position, (i + 1) - point.position);
    }

    result = 1;

    failed:

    if (result == 0) {
        str_free(result_query);
        result_query = NULL;
    }

    return result_query;
}

int __process_value(dbconnection_t* connection, char parameter_type, str_t* string, str_t* value) {
    str_t* quoted_str = NULL;

    if (parameter_type == '@')
        quoted_str = connection->escape_identifier(connection, value);
    else
        quoted_str = connection->escape_string(connection, value);

    if (quoted_str == NULL) return 0;

    str_append(string, str_get(quoted_str), str_size(quoted_str));
    str_free(quoted_str);

    return 1;
}