#define _GNU_SOURCE
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <pthread.h>
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
