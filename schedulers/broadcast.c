#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "broadcast.h"

static broadcast_list_t* broadcast_list = NULL;
static broadcast_list_t* broadcast_list_last = NULL;
static int broadcast_mutex = 0;

broadcast_list_t* __broadcast_create_list(const char* broadcast_name) {
    broadcast_list_t* list = malloc(sizeof * list);
    if (!list) return NULL;

    broadcast_list_t* result = NULL;

    list->item = NULL;
    list->item_last = NULL;
    list->mutex = 0;
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

    if (list->name)
        free(list->name);

    free(list);
}

broadcast_item_t* __broadcast_create_item(connection_t* connection, void* id, int size_id, void(*response_handler)(response_t*, const char*)) {
    broadcast_item_t* item = malloc(sizeof * item);
    if (!item) return NULL;

    broadcast_item_t* result = NULL;

    item->connection = connection;
    item->mutex = 0;
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

    if (item->id)
        free(item->id);

    free(item);
}

broadcast_item_t* __broadcast_lock_item(broadcast_item_t* item) {
    while (1) {
        if (lock(item))
            return item;
    }
    return NULL;
}

broadcast_item_t* __broadcast_lock_first_item(broadcast_list_t* list) {
    return __broadcast_lock_item(list->item);
}

broadcast_item_t* __broadcast_lock_next_item(broadcast_item_t* item) {
    return __broadcast_lock_item(item->next);
}

void __broadcast_unlock_item(broadcast_item_t* item) {
    item->mutex = 0;
}

broadcast_list_t* __broadcast_lock_list(broadcast_list_t* list) {
    while (1) {
        if (lock(list))
            return list;
    }
    return NULL;
}

broadcast_list_t* __broadcast_lock_first_list(broadcast_list_t* list) {
    return __broadcast_lock_list(list);
}

broadcast_list_t* __broadcast_lock_next_list(broadcast_list_t* list) {
    return __broadcast_lock_list(list->next);
}

void __broadcast_unlock_list(broadcast_list_t* list) {
    list->mutex = 0;
}

void __broadcast_remove_list(const char* broadcast_name) {

}

void __broadcast_clear() {

}

broadcast_list_t* __broadcast_get_list(const char* broadcast_name) {
    broadcast_list_t* list = __broadcast_lock_first_list(broadcast_list);
    while (list) {
        if (strcmp(list->name, broadcast_name) == 0)
            return list; // return locked list

        broadcast_list_t* next = __broadcast_lock_next_list(list);

        __broadcast_unlock_list(list);

        list = next;
    }

    return NULL;
}

int __broadcast_list_contains(broadcast_list_t* list, connection_t* connection) {
    broadcast_item_t* item = __broadcast_lock_first_item(list);
    while (item) {
        if (item->connection == connection) {
            __broadcast_unlock_item(item);
            return 1;
        }

        broadcast_item_t* next = __broadcast_lock_next_item(item);

        __broadcast_unlock_item(item);

        item = next;
    }

    return 0;
}

void __broadcast_append_list(broadcast_list_t* list) {
    lock(broadcast_mutex);

    if (!broadcast_list_last) {
        broadcast_list = list;
        broadcast_list_last = list;
        unlock(broadcast_mutex);
        return;
    }

    __broadcast_lock_list(broadcast_list_last);

    broadcast_list_last->next = list;
    broadcast_list_last = list;

    __broadcast_unlock_list(broadcast_list_last);

    unlock(broadcast_mutex);
}

void __broadcast_append_item(broadcast_list_t* list, broadcast_item_t* item) {
    if (!list->item_last) {
        list->item = item;
        list->item_last = item;
        return;
    }

    list->item_last->next = item;
    list->item_last = item;
}

int broadcast_add(const char* broadcast_name, connection_t* connection, void* id, int size_id, void(*response_handler)(response_t*, const char*)) {
    broadcast_list_t* list = __broadcast_get_list(broadcast_name);
    if (!list) {
        list = __broadcast_create_list(broadcast_name);
        if (!list) return 0;

        __broadcast_lock_list(list);
        __broadcast_append_list(list);
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
    broadcast_list_t* list = __broadcast_lock_first_list(broadcast_list);
    while (list) {
        if (strcmp(list->name, broadcast_name) == 0) {
            broadcast_item_t* item = __broadcast_lock_first_item(list);

            if (item && item->connection == connection) {
                list->item = item->next;
                __broadcast_free_item(item);
                __broadcast_unlock_list(list);
                return;
            }

            __broadcast_unlock_list(list);

            while (item) {
                broadcast_item_t* next_item = __broadcast_lock_next_item(item);

                if (next_item->connection == connection) {
                    item->next = next_item->next;
                    __broadcast_free_item(next_item);
                    __broadcast_unlock_item(item);
                    return;
                }

                __broadcast_unlock_item(item);

                item = next_item;
            }

            return;
        }

        broadcast_list_t* next = __broadcast_lock_next_list(list);
        if (!next) return;

        __broadcast_unlock_list(list);

        list = next;
    }
}

void broadcast_send_all(const char* broadcast_name, const char* payload) {
    broadcast_send(broadcast_name, payload, NULL, NULL);
}

void broadcast_send(const char* broadcast_name, const char* payload, void* id, int(*compare_handler)(void* st1, void* st2)) {
    broadcast_list_t* list = __broadcast_lock_first_list(broadcast_list);
    while (list) {
        if (strcmp(list->name, broadcast_name) == 0) {
            broadcast_item_t* item = __broadcast_lock_first_item(list);
            __broadcast_unlock_list(list);
            while (item) {
                if (id && compare_handler && !compare_handler(item->id, id))
                    continue;

                item->response_handler(item->connection->response, payload);

                broadcast_item_t* next_item = __broadcast_lock_next_item(item);

                __broadcast_unlock_item(item);

                item = next_item;
            }

            return;
        }

        broadcast_list_t* next = __broadcast_lock_next_list(list);
        if (!next) return;

        __broadcast_unlock_list(list);

        list = next;
    }
}
