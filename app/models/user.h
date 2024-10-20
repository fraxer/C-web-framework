#ifndef __MODELUSER__
#define __MODELUSER__

#include "model.h"

typedef struct {
    model_t base;
    struct {
        mfield_t id;
        mfield_t email;
        mfield_t name;
        mfield_t enm;
        mfield_t dt;
    } field;
    char table[64];
    char* primary_key[2]; // remove or update by id (primary or unique key)
} user_t;

void* user_instance(void);

user_t* user_get(array_t* params);
int user_create(user_t* user);
int user_update(user_t* user);
int user_delete(user_t* user);

user_t* user_create_anonymous(void);

void user_set_id(user_t* user, int id);
void user_set_name(user_t* user, const char* name);
void user_set_email(user_t* user, const char* email);
void user_set_enum(user_t* user, const char* value);
void user_set_ts(user_t* user, const char* value);

int user_id(user_t* user);
const char* user_name(user_t* user);
const char* user_email(user_t* user);

#endif