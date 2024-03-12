#include <unistd.h>
#include <stdlib.h>

#include "cqueue.h"

cqueue_t* cqueue_create() {
    cqueue_t* queue = malloc(sizeof * queue);
    if (queue == NULL) return NULL;

    queue->locked = 0;
    queue->item = NULL;
    queue->last_item = NULL;

    return queue;
}

void cqueue_free(cqueue_t* queue) {
    cqueue_freecb(queue, NULL);
}

void cqueue_freecb(cqueue_t* queue, void(*free_cb)(void*)) {
    if (queue == NULL) return;

    queue->locked = 0;

    cqueue_item_t* item = queue->item;
    while (item) {
        cqueue_item_t* next_item = item->next;
        if (free_cb != NULL)
            free_cb(item->data);

        cqueue_item_free(item);
        item = next_item;
    }

    queue->locked = 0;

    free(queue);
}

int cqueue_append(cqueue_t* queue, cqueue_item_t* item) {
    if (queue == NULL) return 0;
    if (item == NULL) return 0;

    if (queue->last_item != NULL)
        queue->last_item->next = item;

    queue->last_item = item;

    if (queue->item == NULL)
        queue->item = item;

    return 1;
}

int cqueue_prepend(cqueue_t* queue, cqueue_item_t* item) {
    if (queue == NULL) return 0;
    if (item == NULL) return 0;

    if (queue->last_item == NULL)
        queue->last_item = item;

    item->next = queue->item;

    queue->item = item;

    return 1;
}

void* cqueue_pop(cqueue_t* queue) {
    if (queue == NULL) return NULL;

    cqueue_item_t* item = queue->item;
    if (item) {
        void* data = item->data;
        queue->item = item->next;
        if (queue->item == NULL)
            queue->last_item = NULL;

        cqueue_item_free(item);

        return data;
    }

    return NULL;
}

int cqueue_empty(cqueue_t* queue) {
    if (queue == NULL) return 1;

    return queue->item == NULL;
}

int cqueue_size(cqueue_t* queue) {
    if (queue == NULL) return 0;

    int count = 0;
    cqueue_item_t* item = queue->item;
    while (item) {
        count++;
        item = item->next;
    }

    return count;
}

cqueue_item_t* cqueue_first(cqueue_t* queue) {
    if (queue == NULL) return NULL;

    return queue->item;
}

cqueue_item_t* cqueue_last(cqueue_t* queue) {
    if (queue == NULL) return NULL;

    return queue->last_item;
}

int cqueue_lock(cqueue_t* queue) {
    if (queue == NULL) return 0;

    _Bool expected = 0;
    _Bool desired = 1;

    do {
        expected = 0;
    } while (!atomic_compare_exchange_strong(&queue->locked, &expected, desired));

    return 1;
}

int cqueue_trylock(cqueue_t* queue) {
    if (queue == NULL) return 0;

    _Bool expected = 0;
    _Bool desired = 1;

    return atomic_compare_exchange_strong(&queue->locked, &expected, desired);
}

int cqueue_unlock(cqueue_t* queue) {
    if (queue) {
        atomic_store(&queue->locked, 0);
        return 1;
    }

    return 0;
}

int cqueue_data_remove(cqueue_t* queue, void *addr) {
    if (queue == NULL) return 0;

    int count = 0;
    while (1) {
        cqueue_item_t* item = queue->item;
        if (item && item->data == addr) {
            queue->item = item->next;
            if (queue->item == NULL)
                queue->last_item = NULL;

            cqueue_item_free(item);
            count++;
            continue;
        }

        break;
    }

    cqueue_item_t* item = queue->item;
    while (item) {
        cqueue_item_t* next_item = item->next;

        if (next_item && next_item->data == addr) {
            item->next = next_item->next;
            cqueue_item_free(next_item);
            count++;
            continue;
        }

        item = next_item;
    }

    item = queue->item;
    while (item) {
        queue->last_item = item;
        item = item->next;
    }

    return count;
}

cqueue_item_t* cqueue_item_create(void* data) {
    cqueue_item_t* item = malloc(sizeof * item);
    if (item == NULL) return NULL;

    item->locked = 0;
    item->data = data;
    item->next = NULL;

    return item;
}

void cqueue_item_free(cqueue_item_t* item) {
    if (item == NULL) return;

    free(item);
}

int cqueue_item_lock(cqueue_item_t* item) {
    if (item == NULL) return 0;

    _Bool expected = 0;
    _Bool desired = 1;

    do {
        expected = 0;
    } while (!atomic_compare_exchange_strong(&item->locked, &expected, desired));

    return 1;
}

int cqueue_item_unlock(cqueue_item_t* item) {
    if (item) {
        atomic_store(&item->locked, 0);
        return 1;
    }

    return 0;
}
