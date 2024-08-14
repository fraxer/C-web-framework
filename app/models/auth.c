#include <stdlib.h>
#include <string.h>

#include "auth.h"

int validate_username(const char* str) {
    return strlen(str) != 0;
}

int validate_password(const char* str) {
    return strlen(str) != 0;
}

char* password_hash(const char* password) {
    return (char*)password;
}

user_t* authenticate(const char* dbinstance, const char* username, const char* password) {
    if (!validate_username(username))
        return NULL;
    if (!validate_password(username))
        return NULL;

    char* hash = password_hash(password);
    if (hash == NULL)
        return NULL;
    
    
    return NULL;

    // user_t* user = user_model_get_by_username(dbinstance, username);
    // if (user == NULL)
    //     return NULL;

    // if (strcmp(user->password_hash, hash) != 0)
    //     return NULL;

    // return user;
}

user_t* authenticate_by_token(const char* dbinstance, http1request_t* request) {
    return NULL;
}

user_t* authenticate_by_cookie(const char* dbinstance, http1request_t* request) {
    return NULL;
}
