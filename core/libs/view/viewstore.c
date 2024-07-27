#include <stdlib.h>
#include <string.h>

#include "viewstore.h"
#include "log.h"
#include "helpers.h"

static view_t* __viewstore_view_create(view_tag_t* tag, const char* path);
static void __viewstore_view_free(view_t* view);

/**
 * Create a new view store.
 *
 * @return viewstore_t* The newly created view store, or NULL if allocation failed.
 */
viewstore_t* viewstore_create(void) {
    viewstore_t* store = malloc(sizeof * store);
    if (store == NULL) {
        log_error("viewstore_create: failed to allocate memory for view store");
        return NULL;
    }

    atomic_init(&store->locked, 0);
    store->view = NULL;
    store->last_view = NULL;

    return store;
}

/**
 * Add a view to the view store.
 *
 * @param tag The root tag of the view.
 * @param path The path of the view.
 * @return view_t* The newly added view, or NULL if allocation failed.
 */
view_t* viewstore_add_view(viewstore_t* viewstore, view_tag_t* tag, const char* path) {
    view_t* view = __viewstore_view_create(tag, path);
    if (view == NULL) {
        log_error("viewstore_add_view: failed to allocate memory for view");
        return NULL;
    }

    if (viewstore->view == NULL)
        viewstore->view = view;

    if (viewstore->last_view != NULL)
        viewstore->last_view->next = view;

    viewstore->last_view = view;

    return view;
}

/**
 * Get a view from the view store by path.
 *
 * @param path The path of the view.
 * @return view_t* The view, or NULL if not found.
 */
view_t* viewstore_get_view(viewstore_t* viewstore, const char* path) {
    view_t* view = viewstore->view;
    while (view != NULL) {
        if (cmpstr(view->path, path))
            return view;

        view = view->next;
    }

    return NULL;
}

/**
 * Free a view store.
 *
 * @param viewstore The view store to free.
 * @return void
 */
void viewstore_destroy(viewstore_t* viewstore) {
    if (viewstore == NULL) return;

    view_t* view = viewstore->view;
    while (view != NULL) {
        view_t* next = view->next;
        __viewstore_view_free(view);
        view = next;
    }

    free(viewstore);
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
void viewstore_lock(viewstore_t* viewstore) {
    if (viewstore == NULL) return;

    _Bool expected = 0;
    _Bool desired = 1;

    do {
        expected = 0;
    } while (!atomic_compare_exchange_strong(&viewstore->locked, &expected, desired));
}

/**
 * Unlock the view store.
 *
 * This function unlocks the view store.
 *
 * @return void
 */
void viewstore_unlock(viewstore_t* viewstore) {
    if (viewstore == NULL) return;

    atomic_store(&viewstore->locked, 0);
}
