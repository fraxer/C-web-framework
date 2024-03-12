#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>

#include "log.h"
#include "connection_queue.h"
#include "cqueue.h"

static cqueue_t* queue = NULL;

static pthread_cond_t connection_queue_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t connection_queue_mutex = PTHREAD_MUTEX_INITIALIZER;

void __connection_queue_append(connection_queue_item_t*);
int __connection_queue_empty(cqueue_t*);
connection_t* __connection_queue_pop();
void __connection_queue_item_free(connection_queue_item_t*);
int __connection_queue_append_item(cqueue_t* queue, void* data);


void __connection_queue_append(connection_queue_item_t* qitem) {
    __connection_queue_append_item(qitem->connection->queue, qitem);

    __connection_queue_append_item(queue, qitem->connection);

    atomic_fetch_add_explicit(&qitem->connection->cqueue, 1, memory_order_seq_cst);
}

int __connection_queue_append_item(cqueue_t* queue, void* data) {
    cqueue_item_t* item = cqueue_item_create(data);
    if (item == NULL) {
        log_error("Connection queue error: Can't queue item create\n");
        return 0;
    }

    cqueue_lock(queue);
    cqueue_append(queue, item);
    cqueue_unlock(queue);

    return 1;
}

connection_t* __connection_queue_pop() {
    cqueue_lock(queue);
    connection_t* connection = cqueue_pop(queue);
    cqueue_unlock(queue);

    if (connection == NULL)
        return NULL;

    while (1) {
        if (!connection_lock(connection))
            continue;

        if (!connection_trylockwrite(connection)) {
            connection->queue_pop(connection);
            __connection_queue_append_item(queue, connection);
            connection_unlock(connection);

            return NULL;
        }

        atomic_fetch_add_explicit(&connection->cqueue, -1, memory_order_seq_cst);

        break;
    }

    return connection;
}

int __connection_queue_empty(cqueue_t* queue) {
    if (queue == NULL) return 1;

    cqueue_lock(queue);
    const int empty = cqueue_empty(queue);
    cqueue_unlock(queue);

    return empty;
}

void __connection_queue_item_free(connection_queue_item_t* item) {
    if (item == NULL) return;

    if (item->data != NULL) {
        item->data->free(item->data);
        item->data = NULL;
    }

    free(item);
}

int connection_queue_init() {
    if (queue != NULL) return 1;

    queue = cqueue_create();
    if (queue == NULL) return 0;

    return 1;
}

int connection_queue_lock() {
    return cqueue_lock(queue);
}

int connection_queue_unlock() {
    return cqueue_unlock(queue);
}

void connection_queue_guard_append(connection_queue_item_t* item) {
    pthread_mutex_lock(&connection_queue_mutex);
    __connection_queue_append(item);
    pthread_cond_signal(&connection_queue_cond);
    pthread_mutex_unlock(&connection_queue_mutex);
}

connection_t* connection_queue_guard_pop() {
    if (__connection_queue_empty(queue)) {
        pthread_mutex_lock(&connection_queue_mutex);
        pthread_cond_wait(&connection_queue_cond, &connection_queue_mutex);
        pthread_mutex_unlock(&connection_queue_mutex);
    }

    return __connection_queue_pop();
}

void connection_queue_broadcast() {
    pthread_mutex_lock(&connection_queue_mutex);
    pthread_cond_broadcast(&connection_queue_cond);
    pthread_mutex_unlock(&connection_queue_mutex);
}

connection_queue_item_t* connection_queue_item_create() {
    connection_queue_item_t* item = malloc(sizeof * item);
    if (item == NULL) return NULL;

    item->free = __connection_queue_item_free;
    item->handle = NULL;
    item->connection = NULL;
    item->data = NULL;

    return item;
}

int connection_queue_clear(connection_t* connection) {
    connection_queue_lock();
    const int r = cqueue_data_remove(queue, connection);
    connection_queue_unlock();

    return r;
}
