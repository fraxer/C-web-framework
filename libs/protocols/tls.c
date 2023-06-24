#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "log.h"
#include "connection.h"
#include "protocolmanager.h"
#include "tls.h"

#define TLS_ERROR_ALLOC_SSL "Tls error: can't allocate a new ssl object\n"
#define TLS_ERROR_SET_SSL_FD "Tls error: can't attach fd to ssl\n"
#define TLS_ERROR_WANT_READ "Tls error: ssl want read\n"
#define TLS_ERROR_WANT_WRITE "Tls error: ssl want write\n"
#define TLS_ERROR_WANT_ACCEPT "Tls error: ssl want accept\n"
#define TLS_ERROR_WANT_CONNECT "Tls error: ssl want connect\n"
#define TLS_ERROR_ZERO_RETURN "Tls error: ssl zero return\n"

ssize_t tls_read_internal(connection_t*, char*, size_t);

void tls_read(connection_t* connection, char* buffer, size_t size) {
    (void)buffer;
    (void)size;
    tls_handshake(connection);
}

void tls_write(connection_t* connection, char* buffer, size_t size) {
    (void)connection;
    (void)buffer;
    (void)size;
    log_error("tls write\n");
}

void tls_handshake(connection_t* connection) {
    int result = 0;

    if (connection->ssl == NULL) {

        connection->ssl = SSL_new(connection->server->openssl->ctx);

        if (connection->ssl == NULL) {
            log_error(TLS_ERROR_ALLOC_SSL);
            goto epoll_ssl_error;
        }

        if (!SSL_set_fd(connection->ssl, connection->fd)) {
            log_error(TLS_ERROR_SET_SSL_FD);
            goto epoll_ssl_error;
        }

        SSL_set_accept_state(connection->ssl);
        SSL_set_shutdown(connection->ssl, SSL_SENT_SHUTDOWN | SSL_RECEIVED_SHUTDOWN);
    }

    result = SSL_do_handshake(connection->ssl);

    if (result == 1) {
        protmgr_set_http1(connection);

        return;
    }

    switch (SSL_get_error(connection->ssl, result)) {
    case SSL_ERROR_SYSCALL:
    case SSL_ERROR_SSL:
        epoll_ssl_error:
        connection->close(connection);
        return;

    case SSL_ERROR_WANT_READ:
    case SSL_ERROR_WANT_WRITE:
    case SSL_ERROR_WANT_ACCEPT:
    case SSL_ERROR_WANT_CONNECT:
    case SSL_ERROR_ZERO_RETURN:
        return;
    }
}