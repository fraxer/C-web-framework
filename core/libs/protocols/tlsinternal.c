#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "log.h"
#include "connection.h"
#include "protocolmanager.h"
#include "tlsinternal.h"

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

void tls_client_read(connection_t* connection, char* buffer, size_t size) {
    (void)connection;
    (void)buffer;
    (void)size;
}

void tls_client_write(connection_t* connection, char* buffer, size_t size) {
    (void)buffer;
    (void)size;
    tls_client_handshake(connection);
}

void tls_set_servername(connection_t* connection) {
    const char* server_name = SSL_get_servername(connection->ssl, TLSEXT_NAMETYPE_host_name);
    if (server_name == NULL)
        return;

    union tls_addr {
        struct in_addr ip4;
        struct in6_addr ip6;
    } addrbuf;

    // Per RFC 6066 section 3: ensure that name is not an IP literal.
    if (inet_pton(AF_INET, server_name, &addrbuf) == 1 ||
        inet_pton(AF_INET6, server_name, &addrbuf) == 1)
        return;

    size_t server_name_length = strlen(server_name);
    int vector_struct_size = 6;
    int substring_count = 20;
    int vector_size = substring_count * vector_struct_size;
    int vector[vector_size];

    // Find appropriate SSL context for requested servername.
    for (server_t* server = connection->server; server; server = server->next) {
        if (server->ip != connection->ip || server->port != connection->port) continue;

        for (domain_t* domain = server->domain; domain; domain = domain->next) {
            int matches_count = pcre_exec(domain->pcre_template, NULL, server_name, server_name_length, 0, 0, vector, vector_size);
            if (matches_count > 0) {
                connection->server = server;
                SSL_set_SSL_CTX(connection->ssl, server->openssl->ctx);

#if OPENSSL_VERSION_NUMBER >= 0x009080dfL
                /* only in 0.9.8m+ */
                SSL_clear_options(connection->ssl, SSL_get_options(connection->ssl) & ~SSL_CTX_get_options(server->openssl->ctx));
#endif

                SSL_set_options(connection->ssl, SSL_CTX_get_options(server->openssl->ctx));

#ifdef SSL_OP_NO_RENEGOTIATION
                SSL_set_options(connection->ssl, SSL_OP_NO_RENEGOTIATION);
#endif

                return;
            }
        }
    }
}


void tls_handshake(connection_t* connection) {
    int set_server_name = 0;
    if (connection->ssl == NULL) {
        set_server_name = 1;
        connection->ssl = SSL_new(connection->ssl_ctx);
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

    const int result = SSL_do_handshake(connection->ssl);
    if (result == 1) {
        if (set_server_name)
            tls_set_servername(connection);

        protmgr_set_http1(connection);
        return;
    }

    switch (SSL_get_error(connection->ssl, result)) {
    case SSL_ERROR_SYSCALL:
    case SSL_ERROR_SSL:
        epoll_ssl_error:

        if (errno > 0)
            log_error("tls_handshake: error %d, %d\n", connection->fd, errno);
        return;
    default:
        return;
    }
}

void tls_client_handshake(connection_t* connection) {
    int result = SSL_do_handshake(connection->ssl);
    if (result == 1) {
        protmgr_set_client_http1(connection);
        return;
    }

    switch (SSL_get_error(connection->ssl, result)) {
    case SSL_ERROR_SYSCALL:
    case SSL_ERROR_SSL:
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

void tls_smtp_client_read(connection_t* connection, char* buffer, size_t size) {
    (void)connection;
    (void)buffer;
    (void)size;
}

void tls_smtp_client_write(connection_t* connection, char* buffer, size_t size) {
    (void)buffer;
    (void)size;
    tls_smtp_client_handshake(connection);
}

void tls_smtp_client_handshake(connection_t* connection) {
    int result = SSL_do_handshake(connection->ssl);
    if (result == 1) {
        protmgr_set_smtp_client_command(connection);
        return;
    }

    switch (SSL_get_error(connection->ssl, result)) {
    case SSL_ERROR_SYSCALL:
    case SSL_ERROR_SSL:
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
