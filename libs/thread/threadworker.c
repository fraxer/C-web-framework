#define _GNU_SOURCE
#include <stddef.h>
#include <pthread.h>
#include <stdlib.h>

#include "log.h"
#include "multiplexingserver.h"
#include "threadworker.h"

static void(*__thread_worker_threads_pause)(appconfig_t* config) = NULL;
static void(*__thread_worker_threads_shutdown)(void) = NULL;

void* thread_worker(void* arg) {
    appconfig_t* appconfig = arg;

    appconfg_threads_increment(appconfig);
    appconfg_threads_wait(appconfig);

    if (!mpxserver_run(appconfig, __thread_worker_threads_pause)) {
        __thread_worker_threads_shutdown();
        appconfg_threads_decrement(appconfig);
        pthread_exit(NULL);
    }

    appconfg_threads_decrement(appconfig);

    pthread_exit(NULL);
}

int thread_worker_run(appconfig_t* appconfig, int thread_count) {
    for (int i = 0; i < thread_count; i++) {
        pthread_t thread;
        if (pthread_create(&thread, NULL, thread_worker, appconfig) != 0) {
            log_error("thread_worker_run: unable to create thread worker\n");
            return 0;
        }

        pthread_detach(thread);
        pthread_setname_np(thread, "Server worker");
    }

    return 1;
}

void thread_worker_set_threads_pause_cb(void (*thread_worker_threads_pause)(appconfig_t* config)) {
    if (__thread_worker_threads_pause == NULL)
        __thread_worker_threads_pause = thread_worker_threads_pause;
}

void thread_worker_set_threads_shutdown_cb(void (*thread_worker_threads_shutdown)(void)) {
    if (__thread_worker_threads_shutdown == NULL)
        __thread_worker_threads_shutdown = thread_worker_threads_shutdown;
}
