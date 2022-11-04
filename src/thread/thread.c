#include <stddef.h>
#include <sys/epoll.h>
#include <pthread.h>
#include "thread.h"
#include "../log/log.h"
#include "../epoll/epoll.h"

void* thread_init() {
    return NULL;
}

void* thread_worker(void* arg) {
    // if (fd_handler == THREAD_EPOLL) {
        epoll_run();
    // }
    // if (fd_handler == THREAD_SELECT) {
    //     select_run();
    // }
    // if (fd_handler == THREAD_POLL) {
    //     poll_run();
    // }
    // if (fd_handler == THREAD_KQUEUE) {
    //     kqueue_run();
    // }

    pthread_exit(NULL);
}

void* thread_handler(void* arg) {
    pthread_exit(NULL);
}
