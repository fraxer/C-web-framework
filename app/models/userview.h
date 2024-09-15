#ifndef __USERVIEW__
#define __USERVIEW__

#include "model.h"

typedef struct {
    mfield_t id;
} userview_get_params_t;

typedef struct {
    mfield_t id;
} userview_list_params_t;

typedef struct {
    mfield_t id;
} userview_execute_params_t;

typedef struct {
    modelview_t base;
    struct {
        mfield_t id;
        mfield_t name;
        mfield_t email;
    } field;
} userview_t;

void* userview_instance(void);
userview_t* userview_get(userview_get_params_t* params);
array_t* userview_list();
int userview_execute(userview_execute_params_t* params);

#endif