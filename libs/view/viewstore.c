#include <stdlib.h>
#include <string.h>

#include "viewstore.h"
#include "log.h"
#include "helpers.h"

view_t* __viewstore_view_create(view_tag_t* tag, const char* path);
void __viewstore_view_free(view_t* view);

static viewstore_t* __store = NULL;

/**
 * Returns 1 if viewstore has been initialized, 0 otherwise.
 *
 * @return int 1 if viewstore has been initialized, 0 otherwise.
 */
int viewstore_inited(void) {
    return __store != NULL;
}

/**
 * Initialize the view store and return 1 on success, 0 on failure.
 *
 * @return int 1 on success, 0 on failure.
 */
int viewstore_init(void) {
    viewstore_t* store = malloc(sizeof * __store);
    if (store == NULL) {
        log_error("viewstore_init: failed to allocate memory for view store");
        return 0;
    }

    atomic_init(&store->locked, 0);
    store->view = NULL;
    store->last_view = NULL;
    store->disable_view = NULL;

    __store = store;

    return 1;
}

/**
 * Returns the view store.
 *
 * @return viewstore_t* The view store.
 */
viewstore_t* viewstore(void) {
    return __store;
}

/**
 * Set the view store.
 *
 * @param store The view store to set.
 * @return void
 */
void viewstore_set(viewstore_t* store) {
    __store = store;
}

/**
 * Add a view to the view store.
 *
 * @param tag The root tag of the view.
 * @param path The path of the view.
 * @return view_t* The newly added view, or NULL if allocation failed.
 */
view_t* viewstore_add_view(view_tag_t* tag, const char* path) {
    view_t* view = __viewstore_view_create(tag, path);
    if (view == NULL) {
        log_error("viewstore_add_view: failed to allocate memory for view");
        return NULL;
    }

    if (__store->view == NULL)
        __store->view = view;

    if (__store->last_view != NULL)
        __store->last_view->next = view;

    __store->last_view = view;

    return view;
}

/**
 * Get a view from the view store by path.
 *
 * @param path The path of the view.
 * @return view_t* The view, or NULL if not found.
 */
view_t* viewstore_get_view(const char* path) {
    view_t* view = __store->view;
    while (view != NULL) {
        if (cmpstr(view->path, path))
            return view;

        view = view->next;
    }

    return NULL;
}

/**
 * Clear the view store.
 *
 * @return void
 */
void viewstore_clear(void) {
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

        if (atomic_load(&(view->counter)) == 0) {
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

/**
 * Create a new view.
 *
 * @param tag The root tag of the view.
 * @param path The path of the view.
 * @return view_t* The newly created view, or NULL if allocation failed.
 */
view_t* __viewstore_view_create(view_tag_t* tag, const char* path) {
    view_t* view = malloc(sizeof * view);
    if (view == NULL) {
        log_error("viewstore_create_view: failed to allocate memory for view");
        return NULL;
    }

    atomic_init(&view->counter, 0);
    view->path = malloc(sizeof(char) * (strlen(path) + 1));
    if (view->path == NULL) {
        free(view);
        log_error("viewstore_create_view: failed to allocate memory for view path");
        return NULL;
    }

    strcpy(view->path, path);
    view->root_tag = tag;
    view->next = NULL;

    return view;
}

/**
 * Free a view.
 *
 * @param view The view to free.
 * @return void
 */
void __viewstore_view_free(view_t* view) {
    if (view == NULL) return;

    if (view->path != NULL)
        free(view->path);

    view->root_tag->free(view->root_tag);

    free(view);
}

/**
 * Lock the view store.
 *
 * This function locks the view store to prevent concurrent access to it.
 *
 * @return void
 */
void viewstore_lock(void) {
    if (__store == NULL) return;

    _Bool expected = 0;
    _Bool desired = 1;

    do {
        expected = 0;
    } while (!atomic_compare_exchange_strong(&__store->locked, &expected, desired));
}

/**
 * Unlock the view store.
 *
 * This function unlocks the view store.
 *
 * @return void
 */
void viewstore_unlock(void) {
    if (__store == NULL) return;

    atomic_store(&__store->locked, 0);
}
