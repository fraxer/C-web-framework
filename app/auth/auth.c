#include <stdlib.h>
#include <string.h>

#include "email_validator.h"
#include "password_validator.h"
#include "auth.h"

char* __password_hash(const char* password) {
    return (char*)password;
}

/**
 * @brief Authenticate user by given email and password.
 *
 * @param email Email for authentication.
 * @param password Password for authentication.
 *
 * @return User object if authentication is successful, otherwise NULL.
 */
user_t* authenticate(const char* email, const char* password) {
    if (!validate_email(email))
        return NULL;

    if (!validate_password(password))
        return NULL;

    char* hash = __password_hash(password);
    if (hash == NULL)
        return NULL;

    mparams_create_array(params,
        mparam_text(email, email),
        mparam_text(password, hash)
    );

    user_t* user = user_get(params);
    array_free(params);
    free(hash);

    return user;
}

user_t* authenticate_by_cookie(http1request_t* request) {
    (void)request;

    return NULL;
}
