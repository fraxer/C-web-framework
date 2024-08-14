#ifndef __USERVIEW__
#define __USERVIEW__

#include "model.h"

typedef struct {
    mfield_string_t id;
} userview_get_params_t;

typedef struct {
    modelview_t base;
    struct {
        mfield_int_t id;
        mfield_string_t username;
    } field;
} userview_t;

userview_t* userview_get(userview_get_params_t* params);

#endif