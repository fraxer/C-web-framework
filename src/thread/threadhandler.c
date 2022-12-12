#define _GNU_SOURCE
#include <stddef.h>
#include <pthread.h>
#include <stdlib.h>
#include "threadhandler.h"
#include "../log/log.h"
#include "../connection/connection_queue.h"

typedef struct thread_handler_item {
    int is_deprecated;
    pthread_t thread;
} thread_handler_item_t;

static int thread_handler_count = 0;

static thread_handler_item_t* thread_handlers = NULL;


void* thread_handler(void* arg) {
    thread_handler_item_t* thread_handler = (thread_handler_item_t*)arg;

    while (1) {
        pthread_mutex_lock(&connection_queue_mutex);

        if (connection_queue_empty()) {
            pthread_cond_wait(&connection_queue_cond, &connection_queue_mutex);
        }

        connection_t* connection = connection_queue_pop();

        pthread_mutex_unlock(&connection_queue_mutex);

        if (connection) connection->handle(connection);

        if (thread_handler->is_deprecated) break;
    }

    pthread_exit(NULL);
}

int thread_handler_run(int handler_count) {
    if (thread_handler_count == handler_count) return 0;

    thread_handler_item_t* threads = (thread_handler_item_t*)malloc(sizeof(thread_handler_item_t) * handler_count);

    if (threads == NULL) return -1;

    if (thread_handler_count > handler_count) {
        for (int i = 0; i < handler_count; i++) {
            threads[i].thread = thread_handlers[i].thread;
        }

        pthread_mutex_lock(&connection_queue_mutex);

        for (int i = handler_count; i < thread_handler_count; i++) {
            thread_handlers[i].is_deprecated = 1;
        }

        pthread_cond_broadcast(&connection_queue_cond);

        pthread_mutex_unlock(&connection_queue_mutex);
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

    if (thread_handlers) free(thread_handlers);

    thread_handlers = threads;

    return 0;
}