#ifndef __MODEL_USER_ROLE__
#define __MODEL_USER_ROLE__

#include "model.h"

typedef struct {
    model_t record;
} user_role_t;

void* user_role_instance(void);

user_role_t* user_role_get(array_t* params);
int user_role_create(user_role_t* user_role);
int user_role_update(user_role_t* user_role);
int user_role_delete(user_role_t* user_role);

void user_role_set_user_id(user_role_t* user_role, int user_id);
void user_role_set_role_id(user_role_t* user_role, int role_id);

int user_role_user_id(user_role_t* user_role);
int user_role_role_id(user_role_t* user_role);

#endif
