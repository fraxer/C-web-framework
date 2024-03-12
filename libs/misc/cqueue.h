#ifndef __CQUEUE__
#define __CQUEUE__

#include <stdatomic.h>

typedef struct cqueue_item {
    atomic_bool locked;
    void* data;
    struct cqueue_item* next;
} cqueue_item_t;

typedef struct {
    atomic_bool locked;
    cqueue_item_t* item;
    cqueue_item_t* last_item;
} cqueue_t;

cqueue_t* cqueue_create();
void cqueue_free(cqueue_t*);
void cqueue_freecb(cqueue_t*, void(*free_cb)(void*));
int cqueue_append(cqueue_t*, cqueue_item_t*);
int cqueue_prepend(cqueue_t*, cqueue_item_t*);
void* cqueue_pop(cqueue_t*);
int cqueue_empty(cqueue_t*);
int cqueue_size(cqueue_t*);
cqueue_item_t* cqueue_first(cqueue_t*);
cqueue_item_t* cqueue_last(cqueue_t*);
int cqueue_lock(cqueue_t*);
int cqueue_trylock(cqueue_t*);
int cqueue_unlock(cqueue_t*);
int cqueue_data_remove(cqueue_t*, void*);

cqueue_item_t* cqueue_item_create(void* data);
void cqueue_item_free(cqueue_item_t*);
int cqueue_item_lock(cqueue_item_t*);
int cqueue_item_unlock(cqueue_item_t*);

#endif