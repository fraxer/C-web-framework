#ifndef __OPENSSL__
#define __OPENSSL__

#include <openssl/ssl.h>

typedef struct openssl {
    char* fullchain;
    char* private;
    char* ciphers;
    SSL_CTX* ctx;
} openssl_t;

int openssl_init(openssl_t* openssl);

openssl_t* openssl_create();

void openssl_free(openssl_t*);

#endif