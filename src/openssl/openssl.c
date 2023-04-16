#include <stdlib.h>
#include <stddef.h>
#include <openssl/ssl.h>
#include "../log/log.h"
#include "openssl.h"

#define OPENSSL_ERROR_CHAIN_FILE "Openssl error: can't load chain file\n"
#define OPENSSL_ERROR_PRIVATE_FILE "Openssl error: can't load private file\n"
#define OPENSSL_ERROR_CHECK_PRIVATE_FILE "Openssl error: private file check failed\n"
#define OPENSSL_ERROR_CIPHER_LIST "Openssl error: cipher list is invalid\n"
#define OPENSSL_ERROR_BIO_NEW_FILE "Openssl error: can't create new bio\n"
#define OPENSSL_ERROR_SSL "Openssl error: ssl error\n"
#define OPENSSL_ERROR_WANT_READ "Openssl error: ssl want read\n"
#define OPENSSL_ERROR_WANT_WRITE "Openssl error: ssl want write\n"
#define OPENSSL_ERROR_WANT_ACCEPT "Openssl error: ssl want accept\n"
#define OPENSSL_ERROR_WANT_CONNECT "Openssl error: ssl want connect\n"
#define OPENSSL_ERROR_ZERO_RETURN "Openssl error: ssl zero return\n"

int openssl_context_init(openssl_t*);

openssl_t* openssl_alloc();


int openssl_init(openssl_t* openssl) {
    SSL_library_init();

    if (openssl_context_init(openssl) == -1) return -1;

    return 0;
}

int openssl_context_init(openssl_t* openssl) {
    int result = -1;

    openssl->ctx = SSL_CTX_new(TLS_method());

    if (openssl->ctx == NULL) return -1;

    if (SSL_CTX_use_certificate_chain_file(openssl->ctx, openssl->fullchain) != 1) {
        log_error(OPENSSL_ERROR_CHAIN_FILE);
        goto failed;
    }
    
    if (SSL_CTX_use_PrivateKey_file(openssl->ctx, openssl->private, SSL_FILETYPE_PEM) != 1) {
        log_error(OPENSSL_ERROR_PRIVATE_FILE);
        goto failed;
    }

    if (!SSL_CTX_check_private_key(openssl->ctx)) {
        log_error(OPENSSL_ERROR_CHECK_PRIVATE_FILE);
        goto failed;
    }

    SSL_CTX_set_options(openssl->ctx, SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_RENEGOTIATION);
    SSL_CTX_set_quiet_shutdown(openssl->ctx, 1);

    int dh_enable = 1;

    if (SSL_CTX_set_dh_auto(openssl->ctx, dh_enable) != 1) goto failed;

    if (SSL_CTX_set_cipher_list(openssl->ctx, openssl->ciphers) != 1) {
        log_error(OPENSSL_ERROR_CIPHER_LIST);
        goto failed;
    }

    result = 0;

    failed:

    if (result == -1) {
        openssl_free(openssl);
    }

    return result;
}

openssl_t* openssl_create() {
    openssl_t* openssl = openssl_alloc();

    if (openssl == NULL) return NULL;

    openssl->fullchain = NULL;
    openssl->private = NULL;
    openssl->ciphers = NULL;
    openssl->ctx = NULL;

    return openssl;
}

openssl_t* openssl_alloc() {
    return (openssl_t*)malloc(sizeof(openssl_t));
}

void openssl_free(openssl_t* openssl) {
    if (openssl->fullchain) free(openssl->fullchain);
    openssl->fullchain = NULL;

    if (openssl->private) free(openssl->private);
    openssl->private = NULL;

    if (openssl->ciphers) free(openssl->ciphers);
    openssl->ciphers = NULL;

    if (openssl->ctx) SSL_CTX_free(openssl->ctx);
    openssl->ctx = NULL;

    if (openssl) free(openssl);
}

int openssl_read(SSL* ssl, void* buffer, int num) {
    return SSL_read(ssl, buffer, num);
}

int openssl_write(SSL* ssl, const void* buffer, int num) {
    return SSL_write(ssl, buffer, num);
}

int openssl_get_status(SSL* ssl, int result) {
    int error = SSL_get_error(ssl, result);

    switch (error) {
        case SSL_ERROR_WANT_READ:
            // log_error(OPENSSL_ERROR_WANT_READ);
            return 1;

        case SSL_ERROR_WANT_WRITE:
            // log_error(OPENSSL_ERROR_WANT_WRITE);
            return 1;

        case SSL_ERROR_SYSCALL:
        case SSL_ERROR_SSL:
            // log_error(OPENSSL_ERROR_SSL);
            return 3;

        case SSL_ERROR_WANT_ACCEPT:
            // log_error(OPENSSL_ERROR_WANT_ACCEPT);
            return 4;

        case SSL_ERROR_WANT_CONNECT:
            // log_error(OPENSSL_ERROR_WANT_CONNECT);
            return 5;

        case SSL_ERROR_ZERO_RETURN:
            // log_error(OPENSSL_ERROR_ZERO_RETURN);
            return 6;
    }

    return 0;
}