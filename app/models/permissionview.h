#ifndef __PERMISSIONVIEW__
#define __PERMISSIONVIEW__

#include "model.h"

typedef struct {
    modelview_t base;
    struct {
        mfield_t id;
        mfield_t name;
    } field;
} permissionview_t;

void* permissionview_instance(void);
permissionview_t* permissionview_get(array_t* params);
array_t* permissionview_list(array_t* params);

#endif