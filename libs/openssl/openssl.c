#include <stdlib.h>
#include <stddef.h>

#include "log.h"
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
static int lib_init = 0;

int openssl_init(openssl_t* openssl) {
    if (!lib_init) {
        SSL_library_init();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();
    }
    lib_init = 1;

    if (openssl_context_init(openssl) == -1) return -1;

    return 0;
}

int openssl_context_init(openssl_t* openssl) {
    int result = -1;

    openssl->ctx = SSL_CTX_new(TLS_server_method());
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

    if (SSL_CTX_set_cipher_list(openssl->ctx, openssl->ciphers) != 1) {
        log_error(OPENSSL_ERROR_CIPHER_LIST);
        goto failed;
    }

    result = 0;

    failed:

    return result;
}

openssl_t* openssl_create(void) {
    openssl_t* openssl = malloc(sizeof * openssl);
    if (openssl == NULL) return NULL;

    openssl->fullchain = NULL;
    openssl->private = NULL;
    openssl->ciphers = NULL;
    openssl->ctx = NULL;

    return openssl;
}

void openssl_free(openssl_t* openssl) {
    if (openssl == NULL) return;

    if (openssl->fullchain) free(openssl->fullchain);
    openssl->fullchain = NULL;

    if (openssl->private) free(openssl->private);
    openssl->private = NULL;

    if (openssl->ciphers) free(openssl->ciphers);
    openssl->ciphers = NULL;

    if (openssl->ctx) SSL_CTX_free(openssl->ctx);
    openssl->ctx = NULL;

    free(openssl);
}

int openssl_read(SSL* ssl, void* buffer, int num) {
    return SSL_read(ssl, buffer, num);
}

int openssl_write(SSL* ssl, const void* buffer, size_t num) {
    const int result = SSL_write(ssl, buffer, num);

#ifdef DEBUG
    if (result < 0)
        log_error("openssl_write: errno %d\n", errno);
#endif

    return result;
}
