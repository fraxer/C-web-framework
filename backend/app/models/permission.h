#ifndef __MODELPERMISSION__
#define __MODELPERMISSION__

#include "model.h"

typedef struct {
    model_t record;
} permission_t;

void* permission_instance(void);

permission_t* permission_get(array_t* params);
int permission_create(permission_t* permission);
int permission_update(permission_t* permission);
int permission_delete(permission_t* permission);

void permission_set_id(permission_t* permission, int id);
void permission_set_name(permission_t* permission, const char* name);

int permission_id(permission_t* permission);
const char* permission_name(permission_t* permission);

#endif
