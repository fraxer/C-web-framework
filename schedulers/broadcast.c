#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>

#include "log.h"
#include "broadcast.h"

broadcast_list_t* __broadcast_create_list(const char* broadcast_name) {
    broadcast_list_t* list = malloc(sizeof * list);
    if (!list) return NULL;

    broadcast_list_t* result = NULL;

    list->item = NULL;
    list->item_last = NULL;
    list->locked = 0;
    list->next = NULL;
    list->name = malloc(strlen(broadcast_name) + 1);
    if (!list->name) goto failed;
    strcpy(list->name, broadcast_name);

    result = list;

    failed:

    if (!result)
        free(list);

    return result;
}

void __broadcast_free_list(broadcast_list_t* list) {
    if (!list) return;

    list->item = NULL;
    list->item_last = NULL;
    list->locked = 0;
    list->next = NULL;

    if (list->name)
        free(list->name);

    list->name = NULL;

    free(list);
}

broadcast_item_t* __broadcast_create_item(connection_t* connection, void* id, int size_id, void(*response_handler)(response_t*, const char*, size_t)) {
    broadcast_item_t* item = malloc(sizeof * item);
    if (!item) return NULL;

    broadcast_item_t* result = NULL;

    item->connection = connection;
    item->locked = 0;
    item->next = NULL;
    item->response_handler = response_handler;
    item->id = malloc(size_id);
    if (!item->id) goto failed;
    memcpy(item->id, id, size_id);

    result = item;

    failed:

    if (!result)
        free(item);

    return result;
}

void __broadcast_free_item(broadcast_item_t* item) {
    if (!item) return;

    item->connection = NULL;
    item->locked = 0;
    item->next = NULL;
    item->response_handler = NULL;

    if (item->id)
        free(item->id);

    item->id = NULL;

    free(item);
}

int __broadcast_lock(broadcast_t* broadcast) {
    if (broadcast == NULL) return 0;
    _Bool expected = 0;
    _Bool desired = 1;

    do {
        expected = 0;
        if (broadcast == NULL) return 0;
    } while (!atomic_compare_exchange_strong(&broadcast->locked, &expected, desired));

    return 1;
}

void __broadcast_unlock(broadcast_t* broadcast) {
    if (broadcast == NULL) return;

    atomic_store(&broadcast->locked, 0);
}

broadcast_item_t* __broadcast_lock_item(broadcast_item_t* item) {
    if (item == NULL) return NULL;

    _Bool expected = 0;
    _Bool desired = 1;

    do {
        expected = 0;
        if (item == NULL) return NULL;
    } while (!atomic_compare_exchange_strong(&item->locked, &expected, desired));

    return item;
}

void __broadcast_unlock_item(broadcast_item_t* item) {
    if (item == NULL) return;

    atomic_store(&item->locked, 0);
}

broadcast_list_t* __broadcast_lock_list(broadcast_list_t* list) {
    if (list == NULL) return NULL;

    _Bool expected = 0;
    _Bool desired = 1;

    do {
        expected = 0;
        if (list == NULL) return NULL;
    } while (!atomic_compare_exchange_strong(&list->locked, &expected, desired));

    return list;
}

void __broadcast_unlock_list(broadcast_list_t* list) {
    if (list == NULL) return;

    atomic_store(&list->locked, 0);
}

broadcast_list_t* __broadcast_get_list(const char* broadcast_name, broadcast_list_t* broadcast_list) {
    broadcast_list_t* list = __broadcast_lock_list(broadcast_list);
    while (list) {
        if (strcmp(list->name, broadcast_name) == 0)
            return list; // return locked list

        broadcast_list_t* next = __broadcast_lock_list(list->next);

        __broadcast_unlock_list(list);

        list = next;
    }

    return NULL;
}

int __broadcast_list_contains(broadcast_list_t* list, connection_t* connection) {
    broadcast_item_t* item = __broadcast_lock_item(list->item);
    while (item) {
        if (item->connection == connection) {
            __broadcast_unlock_item(item);
            return 1;
        }

        broadcast_item_t* next = __broadcast_lock_item(item->next);

        __broadcast_unlock_item(item);

        item = next;
    }

    return 0;
}

void __broadcast_append_list(broadcast_t* broadcast, broadcast_list_t* list) {
    if (!__broadcast_lock(broadcast)) return;

    if (!broadcast->list_last) {
        broadcast->list = list;
        broadcast->list_last = list;
        __broadcast_unlock(broadcast);
        return;
    }

    __broadcast_lock_list(broadcast->list_last);

    broadcast->list_last->next = list;
    broadcast->list_last = list;

    __broadcast_unlock_list(broadcast->list_last);
    __broadcast_unlock(broadcast);
}

void __broadcast_append_item(broadcast_list_t* list, broadcast_item_t* item) {
    if (!list->item_last) {
        list->item = item;
        list->item_last = list->item;
        return;
    }

    list->item_last->next = item;
    list->item_last = item;
}

// broadcast_conn_queue_t* __broadcast_conn_queue_lock(broadcast_conn_queue_t* queue) {
//     if (queue == NULL) return NULL;

//     _Bool expected = 0;
//     _Bool desired = 1;

//     do {
//         expected = 0;
//         if (!queue) return NULL;
//     } while (!atomic_compare_exchange_strong(&queue->locked, &expected, desired));

//     return queue;
// }

// void __broadcast_conn_queue_unlock(broadcast_conn_queue_t* queue) {
//     atomic_store(&queue->locked, 0);
// }

// int __broadcast_conn_queue_add(connection_t* connection, broadcast_queue_t* queue) {
//     broadcast_conn_queue_t* item = malloc(sizeof * item);
//     if (item == NULL)
//         return 0;

//     item->next = NULL;
//     item->queue = queue;
//     item->locked = 0;

//     if (connection->broadcast_queue == NULL) {
//         connection->broadcast_queue = item;
//         return 1;
//     }

//     broadcast_conn_queue_t* broadcast_queue = __broadcast_conn_queue_lock(connection->broadcast_queue);
//     while (broadcast_queue) {
//         if (broadcast_queue->next == NULL) {
//             broadcast_queue->next = item;
//             __broadcast_conn_queue_unlock(broadcast_queue);
//             break;
//         }

//         broadcast_conn_queue_t* next = __broadcast_conn_queue_lock(broadcast_queue->next);
//         __broadcast_conn_queue_unlock(broadcast_queue);
//         broadcast_queue = next;
//     }

//     return 1;
// }

// void __broadcast_conn_queue_free(broadcast_conn_queue_t* item) {
//     if (item == NULL) return;

//     item->next = NULL;
//     item->queue = NULL;
//     free(item);
// }

// void __broadcast_conn_queue_remove(connection_t* connection, broadcast_queue_t* queue) {
//     broadcast_conn_queue_t* broadcast_queue = __broadcast_conn_queue_lock(connection->broadcast_queue);
//     if (broadcast_queue == NULL)
//         return;

//     if (broadcast_queue->queue == queue) {
//         connection->broadcast_queue = broadcast_queue->next;
//         __broadcast_conn_queue_free(broadcast_queue);
//         return;
//     }

//     while (broadcast_queue) {
//         broadcast_conn_queue_t* next_queue = __broadcast_conn_queue_lock(broadcast_queue->next);

//         if (next_queue && next_queue->queue == queue) {
//             broadcast_queue->next = next_queue->next;
//             __broadcast_conn_queue_free(next_queue);
//             __broadcast_conn_queue_unlock(broadcast_queue);
//             break;
//         }

//         __broadcast_conn_queue_unlock(broadcast_queue);
//         broadcast_queue = next_queue;
//     }

//     connection->queue_count--;
//     if (!connection_alive(connection) && connection->queue_count == 0)
//         connection_free(connection);
// }

void __broadcast_queue_free(broadcast_queue_t* queue) {
    if (queue == NULL)
        return;

    queue->connection = NULL;
    queue->next = NULL;
    queue->response_handler = NULL;
    queue->size = 0;
    if (queue->payload)
        free(queue->payload);
    queue->payload = NULL;

    free(queue);
}

void __broadcast_queue_add(connection_t* connection, const char* payload, size_t size, void(*response_handler)(response_t* response, const char* payload, size_t size)) {
    broadcast_attrs_t* attrs = ((broadcast_t*)connection->server->broadcast)->broadcast_attrs;

    pthread_mutex_lock(&attrs->scheduler_mutex);

    broadcast_queue_t* queue = malloc(sizeof * queue);
    if (queue == NULL) {
        pthread_mutex_unlock(&attrs->scheduler_mutex);
        return;
    }

    queue->locked = 0;
    queue->connection = connection;
    queue->next = NULL;
    queue->response_handler = response_handler;
    queue->size = size;
    queue->payload = malloc(size);
    if (queue->payload == NULL) {
        __broadcast_queue_free(queue);
        pthread_mutex_unlock(&attrs->scheduler_mutex);
        return;
    }
    memcpy(queue->payload, payload, size);

    // if (!__broadcast_conn_queue_add(connection, queue)) {
    //     __broadcast_queue_free(queue);
    //     pthread_mutex_unlock(&attrs->scheduler_mutex);
    //     return;
    // }

    connection->queue_count++;

    if (attrs->broadcast_queue == NULL)
        attrs->broadcast_queue = queue;

    broadcast_queue_t* queue_last = attrs->broadcast_queue_last;

    if (queue_last)
        queue_last->next = queue;

    attrs->broadcast_queue_last = queue;

    pthread_cond_signal(&attrs->scheduler_cond);
    pthread_mutex_unlock(&attrs->scheduler_mutex);
}

broadcast_t* broadcast_init(broadcast_attrs_t* attrs) {
    broadcast_t* broadcast = malloc(sizeof * broadcast);
    if (!broadcast) return NULL;

    broadcast->broadcast_attrs = attrs;
    broadcast->locked = 0;
    broadcast->list = NULL;
    broadcast->list_last = NULL;

    return broadcast;
}

broadcast_attrs_t* broadcast_attrs_init() {
    broadcast_attrs_t* attrs = malloc(sizeof * attrs);
    if (attrs == NULL) return NULL;

    attrs->broadcast_queue = NULL;
    attrs->broadcast_queue_last = NULL;
    attrs->is_deprecated = 0;
    attrs->thread = 0;
    pthread_cond_init(&attrs->scheduler_cond, NULL);
    pthread_mutex_init(&attrs->scheduler_mutex, NULL);

    return attrs;
}

void broadcast_free(broadcast_t *broadcast) {
    if (!broadcast) return;
    if (!__broadcast_lock(broadcast)) return;

    broadcast_list_t* list = broadcast->list;
    while (list) {
        broadcast_list_t* next = list->next;
        broadcast_item_t* item = __broadcast_lock_item(list->item);
        while (item) {
            broadcast_item_t* next = __broadcast_lock_item(item->next);
            __broadcast_free_item(item);
            item = next;
        }

        __broadcast_free_list(list);
        list = next;
    }

    broadcast->list = NULL;
    broadcast->list_last = NULL;

    free(broadcast);
}

int broadcast_add(const char* broadcast_name, connection_t* connection, void* id, int size_id, void(*response_handler)(response_t* response, const char* payload, size_t size)) {
    broadcast_list_t* list = __broadcast_get_list(broadcast_name, ((broadcast_t*)connection->server->broadcast)->list);
    if (!list) {
        list = __broadcast_create_list(broadcast_name);
        if (!list) return 0;

        __broadcast_lock_list(list);
        __broadcast_append_list((broadcast_t*)connection->server->broadcast, list);
    }

    if (__broadcast_list_contains(list, connection)) {
        __broadcast_unlock_list(list);
        return 0;
    }

    broadcast_item_t* item = __broadcast_create_item(connection, id, size_id, response_handler);
    if (!item) {
        __broadcast_unlock_list(list);
        return 0;
    }

    __broadcast_lock_item(item);
    __broadcast_append_item(list, item);
    __broadcast_unlock_item(item);
    __broadcast_unlock_list(list);

    return 1;
}

void broadcast_remove(const char* broadcast_name, connection_t* connection) {
    broadcast_list_t* list = __broadcast_lock_list(((broadcast_t*)connection->server->broadcast)->list);
    while (list) {
        if (strcmp(list->name, broadcast_name) == 0) {
            broadcast_item_t* item = __broadcast_lock_item(list->item);

            if (item && item->connection == connection) {
                list->item = item->next;
                if (item == list->item_last)
                    list->item_last = NULL;

                __broadcast_free_item(item);
                __broadcast_unlock_list(list);
                return;
            }

            __broadcast_unlock_list(list);

            while (item) {
                broadcast_item_t* next_item = __broadcast_lock_item(item->next);

                if (next_item && next_item->connection == connection) {
                    item->next = next_item->next;
                    if (next_item == list->item_last)
                        list->item_last = item;
                    __broadcast_free_item(next_item);
                    __broadcast_unlock_item(item);
                    return;
                }

                __broadcast_unlock_item(item);

                item = next_item;
            }

            return;
        }

        broadcast_list_t* next = __broadcast_lock_list(list->next);
        __broadcast_unlock_list(list);
        list = next;
    }
}

void broadcast_clear(connection_t* connection) {
    broadcast_list_t* list = __broadcast_lock_list(((broadcast_t*)connection->server->broadcast)->list);
    while (list) {
        broadcast_item_t* item = __broadcast_lock_item(list->item);

        if (item && item->connection == connection) {
            list->item = item->next;
            if (item == list->item_last)
                list->item_last = NULL;

            __broadcast_free_item(item);
            continue;
        }

        while (item) {
            broadcast_item_t* next_item = __broadcast_lock_item(item->next);

            if (next_item && next_item->connection == connection) {
                item->next = next_item->next;
                if (next_item == list->item_last)
                    list->item_last = item;
                __broadcast_free_item(next_item);
                __broadcast_unlock_item(item);
            }

            __broadcast_unlock_item(item);

            item = next_item;
        }

        broadcast_list_t* next = __broadcast_lock_list(list->next);
        __broadcast_unlock_list(list);
        list = next;
    }
}

void broadcast_send_all(const char* broadcast_name, connection_t* connection, const char* payload, size_t size) {
    broadcast_send(broadcast_name, connection, payload, size, NULL, NULL);
}

void broadcast_send(const char* broadcast_name, connection_t* connection, const char* payload, size_t size, void* id, int(*compare_handler)(void* st1, void* st2)) {
    broadcast_list_t* list = __broadcast_lock_list(((broadcast_t*)connection->server->broadcast)->list);
    while (list) {
        if (strcmp(list->name, broadcast_name) == 0) {
            broadcast_item_t* item = __broadcast_lock_item(list->item);
            __broadcast_unlock_list(list);
            while (item) {
                if (connection != item->connection) {
                    if (id && compare_handler && compare_handler(item->id, id))
                        __broadcast_queue_add(item->connection, payload, size, item->response_handler);
                    else if (!id && !compare_handler)
                        __broadcast_queue_add(item->connection, payload, size, item->response_handler);
                }

                broadcast_item_t* next_item = __broadcast_lock_item(item->next);
                __broadcast_unlock_item(item);
                item = next_item;
            }

            return;
        }

        broadcast_list_t* next = __broadcast_lock_list(list->next);
        __broadcast_unlock_list(list);
        list = next;
    }
}

void* broadcast_main(void* arg) {
    broadcast_attrs_t* attrs = arg;
    struct timespec timeToWait = {0};
    struct timeval now;

    while (1) {
        gettimeofday(&now, NULL);
        timeToWait.tv_sec = now.tv_sec + 1;

        pthread_mutex_lock(&attrs->scheduler_mutex);
        pthread_cond_timedwait(&attrs->scheduler_cond, &attrs->scheduler_mutex, &timeToWait);

        broadcast_queue_t* queue = attrs->broadcast_queue;

        attrs->broadcast_queue = NULL;
        attrs->broadcast_queue_last = NULL;

        pthread_mutex_unlock(&attrs->scheduler_mutex);

        if (queue == NULL && attrs->is_deprecated)
            break;

        while (queue) {
            connection_t* connection = queue->connection;
            if (connection_lock(connection) == 0) {
                if (connection_alive(connection)) {
                    if (connection_state(connection) != CONNECTION_WAITREAD) {
                        connection_unlock(connection);
                        continue;
                    }

                    queue->response_handler(connection->response, queue->payload, queue->size);
                    connection->queue_pop(connection);
                }

                connection->queue_count--;
                if (!connection_alive(connection) && connection->queue_count == 0) {
                    connection_free(connection);
                    connection = NULL;
                }

                connection_unlock(connection);
            }

            broadcast_queue_t* next = queue->next;
            __broadcast_queue_free(queue);
            queue = next;
        }
    }

    pthread_exit(NULL);
}

void broadcast_thread(broadcast_attrs_t* broadcast_attrs) {
    if (pthread_create(&broadcast_attrs->thread, NULL, &broadcast_main, broadcast_attrs) != 0) {
        log_error("Thread error: Unable to create worker thread\n");
        return;
    }

    pthread_detach(broadcast_attrs->thread);
    pthread_setname_np(broadcast_attrs->thread, "Broadcast");
}
