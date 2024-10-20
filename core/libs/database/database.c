#define _GNU_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <pthread.h>

#include "log.h"
#include "database.h"

db_t* db_create(const char* db_id) {
    db_t* result = NULL;
    db_t* db = malloc(sizeof * db);
    if (db == NULL) return NULL;

    atomic_store(&db->locked, 0);
    db->circular_index = 0;
    db->id = strdup(db_id);
    db->hosts = array_create();

    if (db->id == NULL) goto failed;
    if (db->hosts == NULL) goto failed;

    result = db;

    failed:

    if (result == NULL)
        db_free(db);

    return result;
}

void db_free(void* arg) {
    if (arg == NULL) return;

    db_t* db = arg;
    array_free(db->hosts);
    free(db->id);
    free(db);
}

void db_cell_free(db_table_cell_t* cell) {
    if (cell == NULL) return;

    if (cell->value != NULL) {
        free(cell->value);
        cell->value = NULL;
    }
}

dbhost_t* db_host_find(db_t* db, const char* host_id) {
    if (host_id == NULL) {
        db_lock(db);
        db->circular_index = db->circular_index + 1 % array_size(db->hosts);
        dbhost_t* host = array_get(db->hosts, db->circular_index);
        db_unlock(db);

        return host;
    }

    for (size_t i = 0; i < array_size(db->hosts); i++) {
        dbhost_t* host = array_get(db->hosts, i);
        if (host == NULL) continue;

        if (strcmp(host->id, host_id) != 0) continue;

        return host;
    }

    return NULL;
}

dbconnection_t* db_connection_find(array_t* connections) {
    for (size_t j = 0; j < array_size(connections); j++) {
        dbconnection_t* connection = array_get(connections, j);
        if (connection == NULL) continue;
        if (connection->thread_id != gettid()) continue;

        return connection;
    }

    return NULL;
}

int host_connections_lock(dbhost_t* host) {
    _Bool expected = 0;
    _Bool desired = 1;

    do {
        expected = 0;
    } while (!atomic_compare_exchange_strong(&host->connections_locked, &expected, desired));

    return 1;
}

void host_connections_unlock(dbhost_t* host) {
    atomic_store(&host->connections_locked, 0);
}


void db_lock(db_t* db) {
    _Bool expected = 0;
    _Bool desired = 1;

    do {
        expected = 0;
    } while (!atomic_compare_exchange_strong(&db->locked, &expected, desired));
}

void db_unlock(db_t* db) {
    atomic_store(&db->locked, 0);
}