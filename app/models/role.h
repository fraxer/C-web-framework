#ifndef __MODELROLE__
#define __MODELROLE__

#include "model.h"

typedef struct {
    model_t base;
    struct {
        mfield_int_t id;
        mfield_string_t name;
    } field;
    char table[64];
    char* primary_key[1]; // remove or update by id (primary or unique key)
} role_t;

typedef struct {
    mfield_string_t id;
} role_get_params_t;


role_t* role_instance(void);
void role_free(void* arg);

role_t* role_get(mfield_t* params, int params_count);
int role_create(role_t* role);
int role_update(role_t* role);
int role_delete(role_t* role);

void role_set_id(role_t* role, int id);
void role_set_name(role_t* role, const char* name);
void role_set_email(role_t* role, const char* email);

int role_id(role_t* role);
const char* role_name(role_t* role);

#endif