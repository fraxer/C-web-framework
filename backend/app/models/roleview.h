#ifndef __ROLEVIEW__
#define __ROLEVIEW__

#include "model.h"

typedef struct {
    model_t record;
} roleview_t;

void* roleview_instance(void);
roleview_t* roleview_get(array_t* params);
array_t* roleview_list(array_t* params);

int roleview_id(roleview_t* role);
const char* roleview_name(roleview_t* role);

#endif
