#ifndef __VIEWSTORE__
#define __VIEWSTORE__

#include "viewcommon.h"

typedef struct {
    atomic_bool locked;
    view_t* view;
    view_t* last_view;
} viewstore_t;

viewstore_t* viewstore_create(void);
view_t* viewstore_add_view(viewstore_t* viewstore, view_tag_t* tag, const char* path);
view_t* viewstore_get_view(viewstore_t* viewstore, const char* path);
void viewstore_destroy(viewstore_t* viewstore);
void viewstore_lock(viewstore_t* viewstore);
void viewstore_unlock(viewstore_t* viewstore);

#endif