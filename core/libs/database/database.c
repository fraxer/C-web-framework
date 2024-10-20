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

// void db_host_free(dbhost_t* host) {
//     while (host != NULL) {
//         dbhost_t* next = host->next;
//         host->free(host);
//         host = next;
//     }
// }

// void db_hosts_free(dbhosts_t* hosts) {
//     if (hosts == NULL) return;

//     db_host_free(hosts->host);
//     free(hosts);
// }

void db_cell_free(db_table_cell_t* cell) {
    if (cell == NULL) return;

    if (cell->value != NULL) {
        free(cell->value);
        cell->value = NULL;
    }
}

dbhost_t* db_host_find(array_t* hosts, const char* host_id) {
    // host_id == NULL then find any available host

    for (size_t i = 0; i < array_size(hosts); i++) {
        dbhost_t* host = array_get(hosts, i);
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
