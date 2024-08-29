#ifndef __ROLEVIEW__
#define __ROLEVIEW__

#include "model.h"

typedef struct {
    mfield_int_t id;
} roleview_get_params_t;

typedef struct {
    mfield_int_t user_id;
} roleview_list_params_t;

typedef struct {
    modelview_t base;
    struct {
        mfield_int_t id;
        mfield_string_t name;
    } field;
} roleview_t;

void* roleview_instance(void);
roleview_t* roleview_get(roleview_get_params_t* params);
array_t* roleview_list(roleview_list_params_t* params);

#endif