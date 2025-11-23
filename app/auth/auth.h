#ifndef __AUTH__
#define __AUTH__

#include "email_validator.h"
#include "password_validator.h"
#include "http.h"
#include "user.h"

#define SALT_SIZE 16
#define HASH_SIZE 32
#define ITERATIONS 140000

int password_hash(const char* password, unsigned char* salt, int salt_size, unsigned char* hash);
int generate_salt(unsigned char* salt, int size);
str_t* generate_secret(const char* password);
str_t* create_secret(int iterations, const char* password_hex, const char* salt_hex);
user_t* authenticate(const char* username, const char* password);
user_t* authenticate_by_cookie(httpctx_t* ctx);

#endif
