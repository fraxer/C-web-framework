#include <stddef.h>
#include <pthread.h>
#include <unistd.h>
#include "../log/log.h"
#include "../server/server.h"
#include "../socket/socket.h"
#include "../connection/connection.h"
#include "epoll.h"
    #include <stdio.h>

static int basefd = 0;

void epoll_run() {
    epoll_event_t events[EPOLL_MAX_EVENTS];

    while(1) {
        int n = epoll_wait(basefd, events, EPOLL_MAX_EVENTS, -1);

        while (--n >= 0) {
            epoll_event_t ev = events[n];

            socket_t* listen_socket = socket_find(ev.data.fd, basefd);

            connection_t* connection = (connection_t*)ev.data.ptr;

            if (listen_socket != NULL) {
                connection = connection_create(listen_socket->fd, basefd, epoll_after_create_connection);

                if (connection == NULL) {
                    log_error("Epoll error: Fail create connection\n");
                }
            }
            else if (ev.events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
                connection->close(connection);
            }
            else if (ev.events & EPOLLIN) {
                connection->read(connection);
            }
            else if (ev.events & EPOLLOUT) {
                connection->write(connection);
            }
        }
    }

    socket_free();

    epoll_close();
}

int epoll_init() {
    basefd = epoll_create1(0);

    if (basefd == -1) {
        log_error("Epoll error: Epoll create1 failed\n");
        return -1;
    }

    for (server_t* server = server_get_first(); server; server = server->next) {
        socket_epoll_t* socket = (socket_epoll_t*)socket_listen_create(basefd, server->ip, server->port, (void*(*)())epoll_socket_alloc);

        if (socket == NULL) return -1;

        socket->event.data.fd = socket->base.fd;
        socket->event.events = EPOLLIN | EPOLLEXCLUSIVE;

        if (epoll_ctl(basefd, EPOLL_CTL_ADD, socket->base.fd, &socket->event) == -1) {
            log_error("Socket error: Epoll_ctl failed in addListener\n");
            return -1;
        }
    }

    return 0;
}

int epoll_close() {
    return close(basefd);
}

int epoll_after_create_connection(connection_t* connection) {
    epoll_event_t* event = (epoll_event_t*)malloc(sizeof(epoll_event_t));

    event->data.ptr = connection;
    event->events = EPOLLIN | EPOLLEXCLUSIVE;

    connection->apidata = event;

    if (epoll_ctl(connection->basefd, EPOLL_CTL_ADD, connection->fd, connection->apidata) == -1) {
        log_error("Socket error: Error epoll_ctl failed accept\n");
        return -1;
    }

    return 0;
}

socket_epoll_t* epoll_socket_alloc() {
    socket_epoll_t* socket = (socket_epoll_t*)malloc(sizeof(socket_epoll_t));

    if (socket == NULL) return NULL;

    socket->base.fd = 0;
    socket->base.next = NULL;

    return socket;
}