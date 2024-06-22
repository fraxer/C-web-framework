#ifndef __VIEWSTORE__
#define __VIEWSTORE__

#include "viewcommon.h"

typedef struct {
    atomic_bool locked;
    view_t* view;
    view_t* last_view;
    view_t* disable_view;
} viewstore_t;

int viewstore_inited(void);
int viewstore_init(void);
viewstore_t* viewstore(void);
void viewstore_set(viewstore_t* store);
view_t* viewstore_add_view(view_tag_t* tag, const char* path);
view_t* viewstore_get_view(const char* path);
void viewstore_clear(void);
void viewstore_lock(void);
void viewstore_unlock(void);

#endif