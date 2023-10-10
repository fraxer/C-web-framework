#include "broadcast.h"

void broadcast_main() {
    // struct timespec timeToWait = {0};
    // struct timeval now;

    while(1) {
        
        // gettimeofday(&now, NULL);

        // timeToWait.tv_sec = now.tv_sec + 1;

        // pthread_mutex_lock(&lock_poll);

        // pthread_cond_timedwait(&cond_poll, &lock_poll, &timeToWait);

        // for (it_p = objs->begin(); it_p != objs->end(); ++it_p) {

        //     bool b = false;

        //     while (!it_p->second->active.compare_exchange_strong(b, true, memory_order_acquire));

        //     if (!it_p->second->is_longpoll) {
        //         it_p->second->active.store(false, memory_order_release);
        //         continue;
        //     }

        //     pthread_mutex_lock(it_p->second->polling->mutex);

        //     if (!it_p->second->polling->ready) {
        //         pthread_mutex_unlock(it_p->second->polling->mutex);
        //         it_p->second->active.store(false, memory_order_release);
        //         continue;
        //     }

        //     it_p->second->polling->gl_counter++;

        //     if (it_p->second->polling->gl_counter > max_seconds) {
        //         it_p->second->status = 504;

        //         Polling::joinConnection(it_p->second->polling->epoll_fd, it_p->first);

        //         it_p->second->polling->ready = false;

        //         pthread_mutex_unlock(it_p->second->polling->mutex);

        //         it_p->second->active.store(false, memory_order_release);

        //         continue;
        //     }

        //     if (!it_p->second->polling->data->size()) {

        //         it_p->second->polling->counter++;

        //         if (it_p->second->polling->counter < max_seconds) {
        //             pthread_mutex_unlock(it_p->second->polling->mutex);
        //             it_p->second->active.store(false, memory_order_release);
        //             continue;
        //         }
        //     }

        //     Polling::makeResult(it_p->second);

        //     Polling::joinConnection(it_p->second->polling->epoll_fd, it_p->first);

        //     it_p->second->polling->ready = false;

        //     pthread_mutex_unlock(it_p->second->polling->mutex);

        //     it_p->second->active.store(false, memory_order_release);
        // }

        // pthread_mutex_unlock(&lock_poll);
    }

    // pthread_exit(NULL);
}
