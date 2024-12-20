#ifndef __MODEL_ROLE_PERMISSION__
#define __MODEL_ROLE_PERMISSION__

#include "model.h"

typedef struct {
    model_t base;
    struct {
        mfield_t role_id;
        mfield_t permission_id;
    } field;
    char table[64];
    char* primary_key[2]; // remove or update by id (primary or unique key)
} role_permission_t;

void* role_permission_instance(void);

role_permission_t* role_permission_get(array_t* params);
int role_permission_create(role_permission_t* role_permission);
int role_permission_update(role_permission_t* role_permission);
int role_permission_delete(role_permission_t* role_permission);

void role_permission_set_role_id(role_permission_t* role_permission, int role_id);
void role_permission_set_permission_id(role_permission_t* role_permission, int user_id);

int role_permission_role_id(role_permission_t* role_permission);
int role_permission_permission_id(role_permission_t* role_permission);

#endif