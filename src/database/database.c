#define _GNU_SOURCE
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <pthread.h>
#include "../log/log.h"
#include "database.h"

db_t* db_alloc() {
    return (db_t*)malloc(sizeof(db_t));
}

const char* db_alloc_id(const char* db_id) {
    char* string = (char*)malloc(strlen(db_id) + 1);

    if (string == NULL) return NULL;

    return strcpy(string, db_id);
}

db_t* db_create(const char* db_id) {
    db_t* result = NULL;
    db_t* db = db_alloc();

    if (db == NULL) return NULL;

    db->id = db_alloc_id(db_id);

    if (db->id == NULL) goto failed;

    atomic_init(&db->lock_connection_read, 0);
    atomic_init(&db->lock_connection_write, 0);
    db->config = NULL;
    db->connection_read = NULL;
    db->connection_write = NULL;
    db->next = NULL;

    result = db;

    failed:

    if (result == NULL) {
        free(db);
    }

    return result;
}

dbhost_t* db_host_create() {
    dbhost_t* host = (dbhost_t*)malloc(sizeof(dbhost_t));

    host->ip = NULL;
    host->port = 0;
    host->read = 0;
    host->write = 0;
    host->next = NULL;

    return host;
}

void db_free(db_t* db) {
    while (db) {
        db_t* next = db->next;

        atomic_store(&db->lock_connection_read, 0);
        atomic_store(&db->lock_connection_write, 0);

        if (db->config != NULL) {
            db->config->free(db->config);
        }
        if (db->connection_read != NULL) {
            db->connection_read->free(db->connection_read);
        }
        if (db->connection_write != NULL) {
            db->connection_write->free(db->connection_write);
        }
        free((void*)db->id);
        free(db);

        db = next;
    }
}

void db_host_free(dbhost_t* host) {
    while (host) {
        dbhost_t* next = host->next;

        host->port = 0;
        host->read = 0;
        host->write = 0;
        if (host->ip) free(host->ip);
        free(host);

        host = next;
    }
}

dbconnection_t* db_find_free_connection(dbconnection_t* connection) {
    while (connection) {
        if (atomic_load(&connection->locked) == 0) {
            _Bool expected = 0;
            _Bool desired = 1;

            if (db_connection_trylock(connection) == 0) {
                return connection;
            }
        }

        connection = connection->next;
    }

    return NULL;
}

void db_connection_append(dbinstance_t* instance, dbconnection_t* connection) {
    _Bool expected = 0;
    _Bool desired = 1;

    do {
        expected = 0;
    } while (!atomic_compare_exchange_strong(instance->lock_connection, &expected, desired));

    if (instance->connection == NULL) {
        instance->connection = connection;
    }
    else {
        dbconnection_t* last_connection = instance->connection;

        while (last_connection) {
            dbconnection_t* next = last_connection->next;

            if (next == NULL) break;

            last_connection = next;
        }

        last_connection->next = connection;
    }

    atomic_store(instance->lock_connection, 0);
}

int db_connection_trylock(dbconnection_t* connection) {
    _Bool expected = 0;
    _Bool desired = 1;

    if (atomic_compare_exchange_strong(&connection->locked, &expected, desired)) return 0;

    return -1;
}

void db_connection_unlock(dbconnection_t* connection) {
    atomic_store(&connection->locked, 0);
}

dbhost_t* db_hosts_load(const jsmntok_t* token_array) {
    dbhost_t* result = NULL;
    dbhost_t* host_first = NULL;
    dbhost_t* host_last = NULL;

    for (jsmntok_t* token_object = token_array->child; token_object; token_object = token_object->sibling) {
        if (token_object->type != JSMN_OBJECT) return NULL;

        dbhost_t* host = db_host_create();

        // check available fields

        for (jsmntok_t* token_item = token_object->child; token_item; token_item = token_item->sibling) {
            const char* key = jsmn_get_value(token_item);

            if (strcmp(key, "ip") == 0) {
                if (token_item->child->type != JSMN_STRING) {
                    log_error("Error database host ip not string\n");
                    goto failed;
                }

                const char* value = jsmn_get_value(token_item->child);

                host->ip = (char*)malloc(strlen(value) + 1);

                if (host->ip == NULL) {
                    log_error("Error database host ip\n");
                    goto failed;
                }

                strcpy(host->ip, value);
            }
            else if (strcmp(key, "port") == 0) {
                if (token_item->child->type != JSMN_PRIMITIVE) {
                    log_error("Error database host port not integer\n");
                    goto failed;
                }

                const char* value = jsmn_get_value(token_item->child);

                host->port = atoi(value);

                if (host->port == 0) {
                    log_error("Error database host port\n");
                    goto failed;
                }
            }
            else if (strcmp(key, "perms") == 0) {
                if (token_item->child->type != JSMN_STRING) {
                    log_error("Error database host perms not string\n");
                    goto failed;
                }

                const char* value = jsmn_get_value(token_item->child);
                const char* p = value;

                while (*p != 0) {
                    if (*p == 'r') {
                        if (host->read) {
                            log_error("Error database host perms duplicate read flag\n");
                            goto failed;
                        }
                        host->read = 1;
                    }
                    else if (*p == 'w') {
                        if (host->write) {
                            log_error("Error database host perms duplicate write flag\n");
                            goto failed;
                        }
                        host->write = 1;
                    }
                    else {
                        log_error("Error database host perms incorrect\n");
                        goto failed;
                    }
                    p++;
                }

                if (!host->read && !host->write) {
                    log_error("Error database host empty perms\n");
                    goto failed;
                }
            }
        }

        // printf("ip %s, port %d, read %d, write %d\n", host->ip, host->port, host->read, host->write);

        if (host_first == NULL) {
            host_first = host;
        }
        if (host_last != NULL) {
            host_last->next = host;
        }
        host_last = host;
    }

    result = host_last;

    failed:

    if (result == NULL) {
        db_host_free(host_first);
    }

    return result;
}
