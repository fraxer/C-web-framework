#ifndef __VIEWSTORE__
#define __VIEWSTORE__

#include "viewcommon.h"

typedef struct {
    atomic_bool locked;
    view_t* view;
    view_t* last_view;
    view_t* disable_view;
} viewstore_t;

int viewstore_init();
viewstore_t* viewstore();
void viewstore_set(viewstore_t* store);
view_t* viewstore_add_view(viewparser_tag_t* tag, const char* path);
view_t* viewstore_get_view(const char* path);
void viewstore_clear();
void viewstore_lock();
void viewstore_unlock();

#endif