#include <stdlib.h>
#include <string.h>

#include "viewstore.h"
#include "helpers.h"

view_t* __viewstore_view_create(viewparser_tag_t* tag, const char* path);
void __viewstore_view_free(view_t* view);

static viewstore_t* __store = NULL;

int viewstore_init() {
    __store = malloc(sizeof * __store);
    if (__store == NULL) return 0;

    atomic_init(&__store->locked, 0);
    __store->view = NULL;
    __store->last_view = NULL;
    __store->disable_view = NULL;

    return 1;
}

viewstore_t* viewstore() {
    return __store;
}

void viewstore_set(viewstore_t* store) {
    __store = store;
}

view_t* viewstore_add_view(viewparser_tag_t* tag, const char* path) {
    view_t* view = __viewstore_view_create(tag, path);
    if (view == NULL) return NULL;

    if (__store->view == NULL)
        __store->view = view;

    if (__store->last_view != NULL)
        __store->last_view->next = view;

    __store->last_view = view;

    return view;
}

view_t* viewstore_get_view(const char* path) {
    view_t* view = __store->view;
    while (view != NULL) {
        if (cmpstr(view->path, path))
            return view;

        view = view->next;
    }

    return NULL;
}

void viewstore_clear() {
    if (__store == NULL) return;

    if (__store->view != NULL) {
        __store->last_view->next = __store->disable_view;
        __store->disable_view = __store->view;
    }

    __store->view = NULL;
    __store->last_view = NULL;

    view_t* view = __store->disable_view;
    view_t* prev = NULL;
    while (view != NULL) {
        view_t* next = view->next;

        if (atomic_load(&view->counter) == 0) {
            if (view == __store->disable_view) {
                __store->disable_view = next;
            }

            __viewstore_view_free(view);

            if (prev != NULL)
                prev->next = next;
        }
        else 
            prev = view;

        view = next;
    }
}

view_t* __viewstore_view_create(viewparser_tag_t* tag, const char* path) {
    view_t* view = malloc(sizeof * view);
    if (view == NULL) return NULL;

    atomic_init(&view->counter, 0);
    view->path = malloc(sizeof(char) * (strlen(path) + 1));
    if (view->path == NULL) {
        free(view);
        return NULL;
    }

    strcpy(view->path, path);
    view->root_tag = tag;
    view->next = NULL;

    return view;
}

void __viewstore_view_free(view_t* view) {
    if (view == NULL) return;

    if (view->path != NULL)
        free(view->path);

    view->root_tag->free(view->root_tag);

    free(view);
}

void viewstore_lock() {
    if (__store == NULL) return;

    _Bool expected = 0;
    _Bool desired = 1;

    do {
        expected = 0;
    } while (!atomic_compare_exchange_strong(&__store->locked, &expected, desired));
}

void viewstore_unlock() {
    if (__store == NULL) return;

    atomic_store(&__store->locked, 0);
}
