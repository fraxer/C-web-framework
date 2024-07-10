#ifndef __OPENSSL__
#define __OPENSSL__

#include <openssl/ssl.h>
#include <openssl/err.h>

typedef struct openssl {
    char* fullchain;
    char* private;
    char* ciphers;
    SSL_CTX* ctx;
} openssl_t;

int openssl_init(openssl_t* openssl);
openssl_t* openssl_create(void);
void openssl_free(openssl_t* openssl);
int openssl_read(SSL*, void*, int);
int openssl_write(SSL*, const void*, size_t);

#endif