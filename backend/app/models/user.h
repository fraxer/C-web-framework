#ifndef __MODELUSER__
#define __MODELUSER__

#include "model.h"

#define USER_SALT_SIZE 32
#define USER_HASH_SIZE 64

typedef struct {
    model_t record;
    char salt[USER_SALT_SIZE + 1];
    char hash[USER_HASH_SIZE + 1];
} user_t;

void* user_instance(void);

user_t* user_get(array_t* params);
int user_create(user_t* user);
int user_update(user_t* user);
int user_delete(user_t* user);
void user_free(user_t* user);

user_t* user_create_anonymous(void);

void user_set_id(user_t* user, int id);
void user_set_name(user_t* user, const char* name);
void user_set_email(user_t* user, const char* email);
void user_set_created_at(user_t* user, const char* value);
void user_set_secret(user_t* user, const char* value);

int user_id(user_t* user);
const char* user_name(user_t* user);
const char* user_email(user_t* user);
const char* user_secret(user_t* user);
const char* user_salt(user_t* user);
const char* user_hash(user_t* user);

#endif
