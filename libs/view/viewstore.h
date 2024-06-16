#ifndef __VIEWSTORE__
#define __VIEWSTORE__

#include "viewcommon.h"

typedef struct {
    view_t* view;
    view_t* last_view;
} viewstore_t;

int viewstore_init();
viewstore_t* viewstore();
void viewstore_set(viewstore_t* store);
view_t* viewstore_add_view(viewparser_tag_t* tag, const char* path);
view_t* viewstore_get_view(const char* path);
void viewstore_clear();

#endif