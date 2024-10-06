#ifndef __MODEL_USER_ROLE__
#define __MODEL_USER_ROLE__

#include "model.h"

typedef struct {
    model_t base;
    struct {
        mfield_t user_id;
        mfield_t role_id;
    } field;
    char table[64];
    char* primary_key[2]; // remove or update by id (primary or unique key)
} user_role_t;

typedef struct {
    mfield_t user_id;
} user_role_get_params_t;

user_role_t* user_role_instance(void);

user_role_t* user_role_get(mfield_t* params, int params_count);
int user_role_create(user_role_t* user_role);
int user_role_update(user_role_t* user_role);
int user_role_delete(user_role_t* user_role);

void user_role_set_user_id(user_role_t* user_role, int user_id);
void user_role_set_role_id(user_role_t* user_role, int role_id);

int user_role_user_id(user_role_t* user_role);
int user_role_role_id(user_role_t* user_role);

#endif