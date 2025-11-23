#ifndef __ROLEVIEW__
#define __ROLEVIEW__

#include "model.h"

typedef struct {
    modelview_t base;
    struct {
        mfield_t id;
        mfield_t name;
    } field;
} roleview_t;

void* roleview_instance(void);
roleview_t* roleview_get(array_t* params);
array_t* roleview_list(array_t* params);

#endif