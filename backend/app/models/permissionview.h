#ifndef __PERMISSIONVIEW__
#define __PERMISSIONVIEW__

#include "model.h"

typedef struct {
    model_t record;
} permissionview_t;

void* permissionview_instance(void);
permissionview_t* permissionview_get(array_t* params);
array_t* permissionview_list(array_t* params);

int permissionview_id(permissionview_t* permission);
const char* permissionview_name(permissionview_t* permission);

#endif
