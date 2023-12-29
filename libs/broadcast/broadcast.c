#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "log.h"
#include "broadcast.h"

typedef struct connection_queue_broadcast_data {
    connection_queue_item_data_t base;
    char* payload;
    size_t size;
    void(*handler)(response_t*, const char*, size_t);
} connection_queue_broadcast_data_t;

void __broadcast_queue_handler(void*);
connection_queue_broadcast_data_t* __broadcast_queue_data_create(const char*, size_t, void(*)(response_t*, const char*, size_t));
void __broadcast_queue_data_free(connection_queue_broadcast_data_t*);

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

broadcast_item_t* __broadcast_create_item(connection_t* connection, void* id, void(*response_handler)(response_t*, const char*, size_t)) {
    broadcast_item_t* item = malloc(sizeof * item);
    if (!item) return NULL;

    item->connection = connection;
    item->locked = 0;
    item->next = NULL;
    item->response_handler = response_handler;
    item->id = id;

    return item;
}

void __broadcast_free_item(broadcast_item_t* item) {
    if (!item) return;

    item->connection = NULL;
    item->locked = 0;
    item->next = NULL;
    item->response_handler = NULL;
    item->id->free(item->id);
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

    if (broadcast->list == NULL)
        broadcast->list = list;

    broadcast_list_t* list_last = broadcast->list_last;

    __broadcast_lock_list(list_last);
    if (list_last)
        list_last->next = list;

    broadcast->list_last = list;

    __broadcast_unlock_list(list_last);
    __broadcast_unlock(broadcast);
}

void __broadcast_append_item(broadcast_list_t* list, broadcast_item_t* item) {
    if (list->item == NULL)
        list->item = item;

    if (list->item_last)
        list->item_last->next = item;

    list->item_last = item;
}

void __broadcast_queue_add(connection_t* connection, const char* payload, size_t size, void(*handle)(response_t* response, const char* payload, size_t size)) {
    connection_queue_item_t* item = connection_queue_item_create();
    if (item == NULL) return;

    item->handle = __broadcast_queue_handler;
    item->connection = connection;
    item->data = (connection_queue_item_data_t*)__broadcast_queue_data_create(payload, size, handle);

    if (item->data == NULL) {
        item->free(item);
        return;
    }

    connection->queue_append(item);
}

void __broadcast_queue_handler(void* arg) {
    connection_queue_item_t* item = arg;
    connection_queue_broadcast_data_t* data = (connection_queue_broadcast_data_t*)item->data;

    data->handler(item->connection->response, data->payload, data->size);
    item->connection->queue_pop(item->connection);
}

connection_queue_broadcast_data_t* __broadcast_queue_data_create(const char* payload, size_t size, void(*handle)(response_t*, const char*, size_t)) {
    connection_queue_broadcast_data_t* data = malloc(sizeof * data);
    if (data == NULL) return NULL;

    data->base.free = free;
    data->payload = malloc(size);
    data->size = size;
    data->handler = handle;

    if (data->payload == NULL) {
        data->base.free(data);
        return NULL;
    }

    memcpy(data->payload, payload, size);

    return data;
}

void __broadcast_queue_data_free(connection_queue_broadcast_data_t* data) {
    if (data == NULL) return;

    if (data->payload) free(data->payload);

    free(data);
}

broadcast_t* broadcast_init() {
    broadcast_t* broadcast = malloc(sizeof * broadcast);
    if (!broadcast) return NULL;

    broadcast->locked = 0;
    broadcast->list = NULL;
    broadcast->list_last = NULL;

    return broadcast;
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

    broadcast->locked = 0;
    broadcast->list = NULL;
    broadcast->list_last = NULL;

    free(broadcast);
}

int broadcast_add(const char* broadcast_name, connection_t* connection, void* id, void(*response_handler)(response_t* response, const char* payload, size_t size)) {
    broadcast_list_t* list = __broadcast_get_list(broadcast_name, connection->server->broadcast->list);
    if (!list) {
        list = __broadcast_create_list(broadcast_name);
        if (!list) return 0;

        __broadcast_lock_list(list);
        __broadcast_append_list(connection->server->broadcast, list);
    }

    if (__broadcast_list_contains(list, connection)) {
        __broadcast_unlock_list(list);
        return 0;
    }

    broadcast_item_t* item = __broadcast_create_item(connection, id, response_handler);
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
    broadcast_list_t* list = __broadcast_lock_list(connection->server->broadcast->list);
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
    broadcast_list_t* list = __broadcast_lock_list(connection->server->broadcast->list);
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
    broadcast_list_t* list = __broadcast_lock_list(connection->server->broadcast->list);
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

    if (id != NULL)
        ((broadcast_id_t*)id)->free(id);
}
