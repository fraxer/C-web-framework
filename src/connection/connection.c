#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "../log/log.h"
#include "../socket/socket.h"
#include "connection.h"

connection_t* connection_create(int listen_socket, int basefd, int(*after_create_connection)(connection_t*)) {
    connection_t* result = NULL;

    struct sockaddr in_addr;

    socklen_t in_len = sizeof(in_addr);

    int connfd = accept(listen_socket, &in_addr, &in_len);

    if (connfd == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) { // Done processing incoming connections 
            return NULL;
        }
        else {
            return NULL;
        }
    }

    if (socket_set_keepalive(connfd) == -1) {
        log_error("Connection error: Error set keepalive\n");
        goto failed;
    }

    if (socket_set_nonblocking(connfd) == -1) {
        log_error("Connection error: Error make_socket_nonblocking failed\n");
        goto failed;
    }

    connection_t* connection = connection_alloc(connfd, basefd);

    if (connection == NULL) goto failed;

    after_create_connection(connection);

    result = connection;

    failed:

    if (result == NULL) {
        close(connfd);
    }

    return result;
}

connection_t* connection_alloc(int fd, int basefd) {
    connection_t* connection = (connection_t*)malloc(sizeof(connection_t));

    if (connection == NULL) return NULL;

    connection->fd = fd;
    connection->basefd = basefd;
    connection->ssl_enabled = 0;
    connection->keepalive_enabled = 0;
    connection->ssl = NULL;
    connection->apidata = NULL;
    connection->close = NULL;
    connection->read = NULL;
    connection->write = NULL;
    connection->after_read_request = NULL;
    connection->after_write_request = NULL;

    pthread_mutex_init(&connection->mutex, NULL);

    return connection;
}

void connection_free(connection_t* connection) {
    pthread_mutex_destroy(&connection->mutex);

    free(connection->apidata);
    free(connection);
}

int connection_trylock(connection_t* connection) {
    return pthread_mutex_trylock(&connection->mutex);
}

int connection_unlock(connection_t* connection) {
    return pthread_mutex_unlock(&connection->mutex);
}