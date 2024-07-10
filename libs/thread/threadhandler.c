#define _GNU_SOURCE
#include <stddef.h>
#include <pthread.h>
#include <stdlib.h>

#include "log.h"
#include "threadhandler.h"
#include "connection_queue.h"

static void(*__thread_handler_threads_pause)(appconfig_t* config) = NULL;

void* thread_handler(void* arg) {
    appconfig_t* appconfig = arg;
    appconfg_threads_increment(appconfig);
    appconfg_threads_wait(appconfig);

    while (1) {
        __thread_handler_threads_pause(appconfig);

        if (atomic_load(&appconfig->shutdown))
            break;

        // connection already locked
        connection_t* connection = connection_queue_guard_pop();
        if (connection == NULL)
            continue;

        connection_queue_item_t* item = cqueue_pop(connection->queue);
        item->handle(item);
        item->free(item);
        if (connection_dec(connection) == CONNECTION_DEC_RESULT_DECREMENT)
            connection_unlock(connection);
    }

    appconfg_threads_decrement(appconfig);

    pthread_exit(NULL);
}

int thread_handler_run(appconfig_t* appconfig, int thread_count) {
    for (int i = 0; i < thread_count; i++) {
        pthread_t thread;
        if (pthread_create(&thread, NULL, thread_handler, appconfig) != 0) {
            log_error("thread_handler_run: unable to create thread handler\n");
            return 0;
        }

        pthread_detach(thread);
        pthread_setname_np(thread, "Server handler");
    }

    return 1;
}

void thread_handlers_wakeup() {
    connection_queue_broadcast();
}

void thread_handler_set_threads_pause_cb(void (*thread_handler_threads_pause)(appconfig_t* config)) {
    if (__thread_handler_threads_pause == NULL)
        __thread_handler_threads_pause = thread_handler_threads_pause;
}
