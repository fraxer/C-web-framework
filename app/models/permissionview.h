#ifndef __PERMISSIONVIEW__
#define __PERMISSIONVIEW__

#include "model.h"

typedef struct {
    mfield_t id;
} permissionview_get_params_t;

typedef struct {
    mfield_t role_id;
} permissionview_list_params_t;

typedef struct {
    modelview_t base;
    struct {
        mfield_t id;
        mfield_t name;
    } field;
} permissionview_t;

void* permissionview_instance(void);
permissionview_t* permissionview_get(permissionview_get_params_t* params);
array_t* permissionview_list(permissionview_list_params_t* params);

#endif