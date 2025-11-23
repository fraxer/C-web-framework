#ifndef __USERVIEW__
#define __USERVIEW__

#include "model.h"

typedef struct {
    modelview_t base;
    struct {
        mfield_t id;
        mfield_t name;
        mfield_t email;
    } field;
} userview_t;

void* userview_instance(void);
userview_t* userview_get(array_t* params);
array_t* userview_list();
int userview_execute(array_t* params);

#endif