#include <string.h>
#include <stdlib.h>

#include "middleware.h"

int run_middlewares(middleware_item_t* middleware_item, void* ctx) {
    while (middleware_item != NULL) {
        if (!middleware_item->fn(ctx))
            return 0;

        middleware_item = middleware_item->next;
    }

    return 1;
}

middleware_item_t* middleware_create(middleware_fn_p fn) {
    if (fn == NULL) return NULL;

    middleware_item_t* item = malloc(sizeof * item);
    if (item == NULL) return NULL;

    item->fn = fn;
    item->next = NULL;

    return item;
}

void middlewares_free(middleware_item_t* middleware_item) {
    while (middleware_item != NULL) {
        middleware_item_t* next = middleware_item->next;
        free(middleware_item);
        middleware_item = next;
    }
}
