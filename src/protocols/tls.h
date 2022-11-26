#ifndef __TLS__
#define __TLS__

#include "../connection/connection.h"
#include "../openssl/openssl.h"

void tls_read(connection_t*, char*, size_t);

void tls_write(connection_t*);

void tls_handshake(connection_t*);

#endif
