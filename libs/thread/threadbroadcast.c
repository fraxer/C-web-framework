#define _GNU_SOURCE
#include <stddef.h>
#include <pthread.h>
#include <stdlib.h>
#include <signal.h>

#include "log.h"
#include "broadcast.h"
#include "threadbroadcast.h"

typedef struct thread_broadcast_item {
    pthread_t thread;
    server_chain_t* server_chain;
} thread_broadcast_item_t;

static int thread_broadcast_index = 0;
static int thread_broadcast_count = 0;

static thread_broadcast_item_t** thread_broadcasts = NULL;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void* thread_broadcast(void* arg) {
    thread_broadcast_item_t* thread_broadcast = (thread_broadcast_item_t*)arg;

    broadcast_run(thread_broadcast->server_chain);

    if (pthread_mutex_trylock(&mutex) == 0) {
        server_chain_t* chain = thread_broadcast->server_chain;
        if (chain && chain->thread_count == 0)
            chain->destroy(chain);

        pthread_mutex_unlock(&mutex);
    }

    if (thread_broadcast) free(thread_broadcast);

    pthread_exit(NULL);
}

thread_broadcast_item_t** thread_broadcast_array_create(int count) {
    thread_broadcast_item_t** threads = malloc(sizeof(thread_broadcast_item_t*) * count);

    if (threads == NULL) return NULL;

    for (int i = 0; i < count; i++) {
        threads[i] = NULL;
    }

    return threads;
}

thread_broadcast_item_t* thread_broadcast_create(server_chain_t* server_chain) {
    thread_broadcast_item_t* item = malloc(sizeof(thread_broadcast_item_t));

    if (item == NULL) return NULL;

    item->server_chain = server_chain;
    item->thread = 0;

    return item;
}

int thread_broadcast_run(int broadcast_count, server_chain_t* server_chain) {
    thread_broadcast_item_t** threads = NULL;

    if (thread_broadcast_count > broadcast_count) {
        // -
        threads = thread_broadcast_array_create(thread_broadcast_count + broadcast_count);
    }
    else {
        // +
        threads = thread_broadcast_array_create(thread_broadcast_count * 2 + broadcast_count);
    }

    if (threads == NULL) return -1;

    for (int i = thread_broadcast_index, j = 0; i < thread_broadcast_index + thread_broadcast_count; i++, j++) {
        if (thread_broadcasts[i])
            threads[j] = thread_broadcasts[i];
    }

    for (int i = thread_broadcast_count; i < thread_broadcast_count + broadcast_count; i++) {
        thread_broadcast_item_t* item = thread_broadcast_create(server_chain);

        if (item == NULL) return -1;

        if (pthread_create(&item->thread, NULL, thread_broadcast, item) != 0) {
            log_error("Thread error: Unable to create broadcast thread\n");
            if (thread_broadcasts) free(thread_broadcasts);
            return -1;
        }

        server_chain->thread_count++;

        pthread_detach(item->thread);

        pthread_setname_np(item->thread, "Broadcast");

        threads[i] = item;
    }

    if (thread_broadcast_count > broadcast_count) {
        // -
        thread_broadcast_index = thread_broadcast_count;

        thread_broadcast_count = broadcast_count;
    }
    else {
        // +
        thread_broadcast_count = thread_broadcast_count + broadcast_count;

        thread_broadcast_index = thread_broadcast_count - broadcast_count;
    }

    if (thread_broadcasts) free(thread_broadcasts);

    thread_broadcasts = threads;

    return 0;
}
