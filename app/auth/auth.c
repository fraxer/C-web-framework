#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#include "helpers.h"
#include "auth.h"
#include "log.h"

/**
 * @brief Hashes a password with a given salt and number of iterations.
 *
 * Uses the PBKDF2 algorithm with the HMAC-SHA256 digest function to hash the password.
 * This is a blocking function and will wait until random bytes are available if necessary.
 *
 * @param[in] password The password to hash.
 * @param[in] salt The salt to use for the hash.
 * @param[in] salt_size The size of the salt.
 * @param[out] hash The buffer where the hash will be stored.
 *
 * @return 1 on success, 0 on failure.
 */
int password_hash(const char* password, unsigned char* salt, int salt_size, unsigned char* hash) {
    if (!PKCS5_PBKDF2_HMAC(password, strlen(password), salt, salt_size, ITERATIONS, EVP_sha256(), HASH_SIZE, hash)) {
        log_error("Error hashing password\n");
        return 0;
    }

    return 1;
}

/**
 * @brief Generates a random salt with the given size.
 *
 * Uses the OpenSSL library's RAND_bytes function to generate a random salt.
 * This function is blocking and will wait until random bytes are available.
 *
 * @param[out] salt The buffer where the salt will be stored.
 * @param[in] size The size of the salt.
 *
 * @return 1 on success, 0 on failure.
 */
int generate_salt(unsigned char* salt, int size) {
    return RAND_bytes(salt, size) == 1;
}

/**
 * @brief Generates a secret string using a password, random salt, and hash.
 *
 * This function generates a random salt and hashes the given password with it
 * using the PBKDF2 algorithm. It then converts the hash to a hexadecimal string
 * and creates a secret by combining the number of iterations, the hash, and the salt.
 *
 * @param[in] password The password to use for generating the secret.
 *
 * @return A pointer to the generated secret string on success, or NULL on failure.
 */
str_t* generate_secret(const char* password) {
    unsigned char salt[SALT_SIZE];
    if (!generate_salt(salt, SALT_SIZE))
        return NULL;

    unsigned char hash[HASH_SIZE];
    if (!password_hash(password, salt, SALT_SIZE, hash))
        return NULL;

    char salt_hex[SALT_SIZE * 2 + 1];
    bytes_to_hex(salt, SALT_SIZE, salt_hex);

    char hash_hex[HASH_SIZE * 2 + 1];
    bytes_to_hex(hash, HASH_SIZE, hash_hex);

    str_t* secret = create_secret(ITERATIONS, hash_hex, salt_hex);
    if (secret == NULL)
        return NULL;

    return secret;
}

/**
 * @brief Creates a secret string combining iterations, password hash, and salt.
 *
 * This function generates a secret string by concatenating the number of iterations,
 * the password hash in hexadecimal format, and the salt in hexadecimal format,
 * separated by the '$' character.
 *
 * @param iterations The number of hash iterations.
 * @param hash_hex The password hash in hexadecimal format.
 * @param salt_hex The salt in hexadecimal format.
 *
 * @return A pointer to the created secret string or NULL if an error occurs.
 */
str_t* create_secret(int iterations, const char* hash_hex, const char* salt_hex) {
    char str[12] = {0};
    ssize_t size = snprintf(str, sizeof(str), "%d", iterations);
    if (size < 0) return NULL;

    str_t* secret = str_create_empty(256);
    if (secret == NULL) return NULL;

    str_append(secret, str, size);
    str_appendc(secret, '$');
    str_append(secret, salt_hex, SALT_SIZE * 2);
    str_appendc(secret, '$');
    str_append(secret, hash_hex, HASH_SIZE * 2);

    return secret;
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

    array_t* params = array_create();
    if (params == NULL)
        return NULL;

    mparams_fill_array(params,
        mparam_text(email, email)
    );
    user_t* user = user_get(params);
    array_free(params);

    if (user == NULL)
        return NULL;

    unsigned char salt[SALT_SIZE];
    if (!hex_to_bytes(user_salt(user), salt))
        return NULL;

    unsigned char hash[HASH_SIZE];
    if (!password_hash(password, salt, SALT_SIZE, hash))
        return NULL;

    char hash_hex[HASH_SIZE * 2 + 1];
    bytes_to_hex(hash, HASH_SIZE, hash_hex);

    if (strcmp(user_hash(user), hash_hex) != 0)
        return NULL;

    return user;
}

/**
 * @brief Authenticate user by given HTTP cookie.
 *
 * @param ctx The HTTP context for authentication.
 *
 * @return User object if authentication is successful, otherwise NULL.
 */
user_t* authenticate_by_cookie(httpctx_t* ctx) {
    const char* token = ctx->request->get_cookie(ctx->request, "token");
    if (token == NULL)
        return NULL;

    array_t* params = array_create();
    if (params == NULL)
        return NULL;

    mparams_fill_array(params,
        mparam_text(token, token)
    );
    user_t* user = user_get(params);
    array_free(params);

    return user;
}
