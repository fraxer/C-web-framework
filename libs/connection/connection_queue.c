#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>

#include "log.h"
#include "connection_queue.h"

typedef struct connection_queue {
    connection_t* connection;
    struct connection_queue* next;
} connection_queue_t;

static connection_queue_t* queue = NULL;
static connection_queue_t* last_queue = NULL;

static pthread_cond_t connection_queue_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t connection_queue_mutex = PTHREAD_MUTEX_INITIALIZER;

connection_queue_t* connection_queue_alloc();

connection_queue_t* connection_queue_create(connection_t*);

void connection_queue_destroy(connection_queue_t*);


void connection_queue_push(connection_t* connection) {
    connection_queue_t* queue_item = connection_queue_create(connection);

    if (queue_item == NULL) return;

    if (queue == NULL) {
        queue = queue_item;
    }

    if (last_queue == NULL) {
        last_queue = queue;
    } else {
        last_queue->next = queue_item;
        last_queue = queue_item;
    }
}

void connection_queue_guard_push(connection_t* connection) {
    pthread_mutex_lock(&connection_queue_mutex);

    connection_queue_push(connection);

    pthread_cond_signal(&connection_queue_cond);

    pthread_mutex_unlock(&connection_queue_mutex);
}

connection_t* connection_queue_pop() {
    connection_queue_t* queue_item = queue;

    if (queue_item == NULL) return NULL;

    queue = queue->next;

    if (queue == NULL) {
        last_queue = NULL;
    }

    connection_t* connection = queue_item->connection;

    connection_queue_destroy(queue_item);

    return connection;
}

connection_t* connection_queue_guard_pop() {
    pthread_mutex_lock(&connection_queue_mutex);

    if (connection_queue_empty()) {
        pthread_cond_wait(&connection_queue_cond, &connection_queue_mutex);
    }

    connection_t* connection = connection_queue_pop();

    pthread_mutex_unlock(&connection_queue_mutex);

    return connection;
}

void connection_queue_destroy(connection_queue_t* queue_item) {
    free(queue_item);
    queue_item = NULL;
}

int connection_queue_empty() {
    return queue == NULL;
}

connection_queue_t* connection_queue_alloc() {
    return (connection_queue_t*)malloc(sizeof(connection_queue_t));
}

connection_queue_t* connection_queue_create(connection_t* connection) {
    connection_queue_t* queue_item = connection_queue_alloc();

    queue_item->connection = connection;
    queue_item->next = NULL;

    return queue_item;
}

void connection_queue_broadcast() {
    pthread_mutex_lock(&connection_queue_mutex);

    pthread_cond_broadcast(&connection_queue_cond);

    pthread_mutex_unlock(&connection_queue_mutex);
}