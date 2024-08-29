#ifndef __MODELUSER__
#define __MODELUSER__

#include "model.h"

typedef struct {
    model_t base;
    struct {
        mfield_int_t id;
        mfield_string_t email;
        mfield_string_t name;
    } field;
    char table[64];
    char* primary_key[2]; // remove or update by id (primary or unique key)
} user_t;

typedef struct {
    mfield_string_t id;
} user_get_params_t;


user_t* user_instance(void);
void user_free(void* arg);

user_t* user_get(mfield_t* params, int params_count);
int user_create(user_t* user);
int user_update(user_t* user);
int user_delete(user_t* user);

user_t* user_create_anonymous(void);

void user_set_id(user_t* user, int id);
void user_set_name(user_t* user, const char* name);
void user_set_email(user_t* user, const char* email);

int user_id(user_t* user);
const char* user_name(user_t* user);
char* user_stringify(user_t* user);

#endif