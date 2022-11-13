#define _GNU_SOURCE
#include <stddef.h>
#include <pthread.h>
#include <stdlib.h>
#include "threadhandler.h"
#include "../log/log.h"

typedef struct thread_queue {
    connection_t* connection;
    struct thread_queue* next;
} thread_queue_t;

typedef struct thread_handler_item {
    int is_deprecated;
    pthread_t thread;
} thread_handler_item_t;

static int thread_handler_count = 0;

static thread_handler_item_t* thread_handlers = NULL;

static pthread_cond_t thread_queue_cond = PTHREAD_COND_INITIALIZER;

static pthread_mutex_t thread_queue_mutex = PTHREAD_MUTEX_INITIALIZER;

static thread_queue_t* queue = NULL;
static thread_queue_t* last_in_queue = NULL;

thread_queue_t* thread_queue_wait_unshift();

void thread_queue_free(thread_queue_t*);

void* thread_handler(void* arg) {
    while (1) {
        thread_queue_t* queue_item = thread_queue_wait_unshift();

        if (queue_item) {
            queue_item->connection->handle(queue_item->connection);

            thread_queue_free(queue_item);
        }
    }

    pthread_exit(NULL);
}

thread_queue_t* thread_queue_alloc() {
    return (thread_queue_t*)malloc(sizeof(thread_queue_t));
}

thread_queue_t* thread_queue_create(connection_t* connection) {
    thread_queue_t* queue = thread_queue_alloc();

    if (queue == NULL) return NULL;

    queue->connection = connection;
    queue->next = NULL;

    return queue;
}

void thread_queue_push(connection_t* connection) {
    thread_queue_t* queue_item = thread_queue_create(connection);

    if (queue_item == NULL) return;

    pthread_mutex_lock(&thread_queue_mutex);

    if (last_in_queue) {
        last_in_queue->next = queue_item;
    } else {
        queue = queue_item;
        last_in_queue = queue_item;
    }

    pthread_cond_signal(&thread_queue_cond);

    pthread_mutex_unlock(&thread_queue_mutex);
}

thread_queue_t* thread_queue_wait_unshift() {
    pthread_mutex_lock(&thread_queue_mutex);

    if (queue == NULL) {
        pthread_cond_wait(&thread_queue_cond, &thread_queue_mutex);
    }

    if (queue == NULL) return NULL;

    thread_queue_t* queue_item = queue;

    queue = queue->next;

    if (queue == NULL) {
        last_in_queue = NULL;
    }

    pthread_mutex_unlock(&thread_queue_mutex);

    queue_item->next = NULL;

    return queue_item;
}

void thread_queue_free(thread_queue_t* queue_item) {
    free(queue_item);
}

int thread_handler_run(int handler_count) {
    if (thread_handler_count == handler_count) return 0;

    thread_handler_item_t* threads = (thread_handler_item_t*)malloc(sizeof(thread_handler_item_t) * handler_count);

    if (threads == NULL) return -1;

    if (thread_handler_count > handler_count) {
        for (int i = 0; i < handler_count; i++) {
            threads[i].thread = thread_handlers[i].thread;
        }

        pthread_mutex_lock(&thread_queue_mutex);

        for (int i = handler_count; i < thread_handler_count; i++) {
            thread_handlers[i].is_deprecated = 1;
        }

        pthread_cond_broadcast(&thread_queue_cond);

        pthread_mutex_unlock(&thread_queue_mutex);
    }
    else {
        for (int i = 0; i < thread_handler_count; i++) {
            threads[i] = thread_handlers[i];
        }

        for (int i = thread_handler_count; i < handler_count; i++) {
            if (pthread_create(&threads[i].thread, NULL, thread_handler, &threads[i].is_deprecated) != 0) {
                log_error("Thread error: Unable to create handler thread\n");
                return -1;
            }

            pthread_setname_np(threads[i].thread, "Server handler");
        }
    }

    thread_handler_count = handler_count;

    pthread_mutex_lock(&thread_queue_mutex);

    if (thread_handlers) free(thread_handlers);

    pthread_mutex_unlock(&thread_queue_mutex);

    thread_handlers = threads;

    return 0;
}