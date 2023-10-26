#include <sys/time.h>

#include "broadcast.h"

// void broadcast_main() {
//     struct timespec timeToWait = {0};
//     struct timeval now;

//     while (1) {
//         gettimeofday(&now, NULL);
//         timeToWait.tv_sec = now.tv_sec + 1;

//         pthread_mutex_lock(&scheduler_mutex);
//         pthread_cond_timedwait(&scheduler_cond, &scheduler_mutex, &timeToWait);

//         broadcast_queue_t* queue = broadcast_queue();

//         pthread_mutex_unlock(&scheduler_mutex);

//         while (queue) {
//             if (connection_trylock(queue->connection) != 0) {
//                 continue;
//             }

//             queue->response_handler(queue->connection->response, queue->payload);
//             queue->connection->queue_pop(queue->connection);

//             connection_unlock(queue->connection);
//         }
//     }

//     pthread_exit(NULL);
// }
