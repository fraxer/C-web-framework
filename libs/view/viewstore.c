#include <stdlib.h>
#include <string.h>

#include "viewstore.h"
#include "helpers.h"

view_t* __viewstore_view_create(viewparser_tag_t* tag, const char* path);
void __viewstore_view_free(view_t* view);
void __viewstore_tag_free(viewparser_tag_t* tag);

static viewstore_t* __store = NULL;

int viewstore_init() {
    __store = malloc(sizeof * __store);
    if (__store == NULL) return 0;

    __store->view = NULL;
    __store->last_view = NULL;

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
    view_t* view = __store->view;
    while (view != NULL) {
        view_t* next = view->next;
        __viewstore_view_free(view);
        view = next;
    }
}

view_t* __viewstore_view_create(viewparser_tag_t* tag, const char* path) {
    view_t* view = malloc(sizeof * view);
    if (view == NULL) return NULL;

    view->path = malloc(sizeof(char) * (strlen(path) + 1));
    if (view->path == NULL) {
        free(view);
        return NULL;
    }

    bufferdata_init(&view->buf);
    strcpy(view->path, path);
    view->root_tag = tag;
    view->next = NULL;

    return view;
}

void __viewstore_view_free(view_t* view) {
    if (view == NULL) return;

    bufferdata_clear(&view->buf);
    if (view->path != NULL)
        free(view->path);

    __viewstore_tag_free(view->root_tag);
    // __viewstore_tag_copy_free(view->root_tag);

    free(view);
}

void __viewstore_tag_free(viewparser_tag_t* tag) {
    if (tag == NULL) return;

    bufferdata_clear(&tag->result_content);
    tag->data_parent = NULL;

    // struct viewparser_tag* child;
    // struct viewparser_tag* last_child;
    // struct viewparser_tag* next;

    // viewparser_variable_item_t* item;
    // viewparser_variable_item_t* last_item;
}
