#ifndef __MODELPERMISSION__
#define __MODELPERMISSION__

#include "model.h"

typedef struct {
    model_t base;
    struct {
        mfield_t id;
        mfield_t name;
    } field;
    char table[64];
    char* primary_key[1]; // remove or update by id (primary or unique key)
} permission_t;

typedef struct {
    mfield_t id;
} permission_get_params_t;

permission_t* permission_instance(void);

permission_t* permission_get(mfield_t* params, int params_count);
int permission_create(permission_t* permission);
int permission_update(permission_t* permission);
int permission_delete(permission_t* permission);

void permission_set_id(permission_t* permission, int id);
void permission_set_name(permission_t* permission, const char* name);
void permission_set_email(permission_t* permission, const char* email);

int permission_id(permission_t* permission);
const char* permission_name(permission_t* permission);

#endif