#ifndef __USERVIEW__
#define __USERVIEW__

#include "model.h"

typedef struct {
    model_t record;
} userview_t;

void* userview_instance(void);
userview_t* userview_get(array_t* params);
array_t* userview_list();
int userview_execute(array_t* params);

int userview_id(userview_t* user);
const char* userview_name(userview_t* user);
const char* userview_email(userview_t* user);

#endif
