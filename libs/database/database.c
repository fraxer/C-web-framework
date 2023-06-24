#define _GNU_SOURCE
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <pthread.h>

#include "log.h"
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

    atomic_init(&db->lock_connection, 0);
    db->hosts = NULL;
    db->connection = NULL;
    db->next = NULL;

    result = db;

    failed:

    if (result == NULL) {
        free(db);
    }

    return result;
}

dbhost_t* db_host_create() {
    dbhost_t* host = malloc(sizeof *host);

    host->free = NULL;
    host->next = NULL;

    return host;
}

void db_next_host(dbhosts_t* hosts) {
    if (hosts->current_host->next != NULL) {
        hosts->current_host = hosts->current_host->next;
        return;
    }

    hosts->current_host = hosts->host;
}

void db_free(db_t* db) {
    while (db) {
        db_t* next = db->next;

        atomic_store(&db->lock_connection, 0);

        if (db->hosts != NULL) {
            db_hosts_free(db->hosts);
        }
        if (db->connection != NULL) {
            db->connection->free(db->connection);
        }
        free((void*)db->id);
        free(db);

        db = next;
    }
}

void db_host_free(dbhost_t* host) {
    while (host) {
        dbhost_t* next = host->next;
        host->free(host);
        host = next;
    }
}

void db_hosts_free(dbhosts_t* hosts) {
    db_host_free(hosts->host);

    hosts->host = NULL;
    hosts->current_host = NULL;
    hosts->connection_create = NULL;

    free(hosts);
}

void db_cell_free(db_table_cell_t* cell) {
    if (cell == NULL) return;

    if (cell->value) {
        free(cell->value);
        cell->value = NULL;
    }
}

dbconnection_t* db_connection_find(dbconnection_t* connection) {
    while (connection) {
        if (db_connection_trylock(connection) == 0) {
            return connection;
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

    if (*instance->connection == NULL) {
        *instance->connection = connection;
    }
    else {
        dbconnection_t* last_connection = *instance->connection;

        while (last_connection) {
            dbconnection_t* next = last_connection->next;

            if (next == NULL) break;

            last_connection = next;
        }

        last_connection->next = connection;
    }

    atomic_store(instance->lock_connection, 0);
}

void db_connection_pop(dbinstance_t* instance, dbconnection_t* connection) {
    _Bool expected = 0;
    _Bool desired = 1;

    do {
        expected = 0;
    } while (!atomic_compare_exchange_strong(instance->lock_connection, &expected, desired));

    if (*instance->connection != NULL) {
        dbconnection_t* curr_connection = *instance->connection;
        dbconnection_t* prev_connection = NULL;

        while (curr_connection) {
            if (connection == curr_connection) break;

            prev_connection = curr_connection;

            curr_connection = curr_connection->next;
        }

        if (curr_connection) {
            dbconnection_t* next_connection = curr_connection->next;

            if (prev_connection) {
                db_connection_lock(prev_connection);

                prev_connection->next = next_connection;

                db_connection_unlock(prev_connection);
            }
        }

        curr_connection->next = NULL;
    }

    atomic_store(instance->lock_connection, 0);
}

int db_connection_trylock(dbconnection_t* connection) {
    _Bool expected = 0;
    _Bool desired = 1;

    if (atomic_compare_exchange_strong(&connection->locked, &expected, desired)) return 0;

    return -1;
}

int db_connection_lock(dbconnection_t* connection) {
    _Bool expected = 0;
    _Bool desired = 1;

    do {
        expected = 0;
    } while (!atomic_compare_exchange_strong(&connection->locked, &expected, desired));

    return 1;
}

void db_connection_unlock(dbconnection_t* connection) {
    atomic_store(&connection->locked, 0);
}

void db_connection_free(dbconnection_t* connection) {
    connection->free(connection);
}
