#include <unistd.h>
#include <errno.h>

#include "log.h"
#include "openssl.h"
#include "connection.h"
#include "multiplexing.h"

void broadcast_clear(connection_t*);
void __connection_free_queue_item(void* arg);

connection_t* connection_server_create(connection_t* socket_connection) {
    connection_t* result = NULL;
    struct sockaddr in_addr;
    socklen_t in_len = sizeof(in_addr);

    #pragma GCC diagnostic ignored "-Wanalyzer-fd-leak"
    const int connfd = accept(socket_connection->fd, &in_addr, &in_len);
    if (connfd == -1)
        return NULL;

    if (socket_set_keepalive(connfd) == -1) {
        log_error("Connection error: Error set keepalive\n");
        goto failed;
    }

    if (socket_set_nonblocking(connfd) == -1) {
        log_error("Connection error: Error make_socket_nonblocking failed\n");
        goto failed;
    }

    connection_t* connection = connection_alloc(connfd, socket_connection->api, socket_connection->ip, socket_connection->port);
    if (connection == NULL) goto failed;

    result = connection;

    failed:

    if (result == NULL)
        close(connfd);

    return result;
}

connection_t* connection_client_create(const int fd, const in_addr_t ip, const short port) {
    return connection_alloc(fd, NULL, ip, port);
}

connection_t* connection_alloc(int fd, mpxapi_t* api, in_addr_t ip, unsigned short int port) {
    connection_t* connection = malloc(sizeof(connection_t));
    if (connection == NULL) return NULL;

    connection->fd = fd;
    connection->api = api;
    connection->keepalive_enabled = 0;
    connection->cqueue = 0;
    connection->ip = ip;
    connection->port = port;
    atomic_store(&connection->locked, 0);
    atomic_store(&connection->onwrite, 0);
    connection->ssl = NULL;
    connection->ssl_ctx = NULL;
    connection->server = NULL;
    connection->client = NULL;
    connection->request = NULL;
    connection->response = NULL;
    connection->close = NULL;
    connection->read = NULL;
    connection->write = NULL;
    connection->after_read_request = NULL;
    connection->after_write_request = NULL;
    connection->queue_append = NULL;
    connection->queue_append_broadcast = NULL;
    connection->queue_pop = NULL;
    connection->switch_to_protocol = NULL;
    connection->queue = cqueue_create();

    if (!gzip_init(&connection->gzip)) {
        free(connection);
        return NULL;
    }
    if (connection->queue == NULL) {
        free(connection);
        return NULL;
    }

    return connection;
}

void __connection_free_queue_item(void* arg) {
    connection_queue_item_t* item = arg;
    item->free(item);
}

void connection_free(connection_t* connection) {
    if (connection == NULL) return;

    broadcast_clear(connection);

    gzip_free(&connection->gzip);

    if (connection->ssl != NULL) {
        SSL_free_buffers(connection->ssl);
        SSL_free(connection->ssl);
        connection->ssl = NULL;
    }

    if (connection->request != NULL) {
        connection->request->free(connection->request);
        connection->request = NULL;
    }

    if (connection->response != NULL) {
        connection->response->free(connection->response);
        connection->response = NULL;
    }

    cqueue_freecb(connection->queue, __connection_free_queue_item);
    connection->queue = NULL;

    free(connection);
}

void connection_reset(connection_t* connection) {
    if (connection == NULL) return;

    gzip_reset(&connection->gzip);

    if (connection->request != NULL)
        connection->request->reset(connection->request);

    if (connection->response != NULL)
        connection->response->reset(connection->response);
}

int connection_trylock(connection_t* connection) {
    if (connection == NULL) return 0;

    _Bool expected = 0;
    _Bool desired = 1;

    if (atomic_compare_exchange_strong(&connection->locked, &expected, desired)) return 1;

    return 0;
}

int connection_lock(connection_t* connection) {
    if (connection == NULL) return 0;

    _Bool expected = 0;
    _Bool desired = 1;

    do {
        expected = 0;
    } while (!atomic_compare_exchange_strong(&connection->locked, &expected, desired));

    return 1;
}

int connection_unlock(connection_t* connection) {
    if (connection) {
        atomic_store(&connection->locked, 0);
        return 1;
    }

    return 0;
}

int connection_trylockwrite(connection_t* connection) {
    if (connection == NULL) return 0;

    _Bool expected = 0;
    _Bool desired = 1;

    if (atomic_compare_exchange_strong(&connection->onwrite, &expected, desired)) return 1;

    return 0;
}
