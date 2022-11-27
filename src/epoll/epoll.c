#define _GNU_SOURCE
#include <stddef.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <sys/epoll.h>
#include "../log/log.h"
#include "../server/server.h"
#include "../socket/socket.h"
#include "../connection/connection.h"
#include "../protocols/protocolmanager.h"
#include "../protocols/http1.h"
#include "../protocols/websockets.h"
#include "epoll.h"
    #include <stdio.h>

#define EPOLL_MAX_EVENTS 16
#define EPOLL_BUFFER 16384

typedef struct epoll_event epoll_event_t;

typedef struct socket_epoll {
    socket_t base;
    epoll_event_t* event;
} socket_epoll_t;

int epoll_init(socket_epoll_t** first_socket, server_t*);

int epoll_after_create_connection(connection_t*, server_chain_t*, server_t*);

int epoll_after_read_request(connection_t*);

int epoll_after_write_request(connection_t*);

int epoll_connection_close(connection_t*);

socket_epoll_t* epoll_socket_alloc();

int epoll_control_add(connection_t*, uint32_t);

int epoll_control_mod(connection_t*, uint32_t);

int epoll_control_del(connection_t*);

char* epoll_buffer_alloc();

int epoll_connection_set_event(connection_t*);

void epoll_connection_set_hooks(connection_t*);

int epoll_disable(socket_t*, int);


void epoll_run(void* chain) {
    int result = -1;

    server_chain_t* server_chain = chain;

    socket_epoll_t* first_socket = NULL;

    int basefd = epoll_init(&first_socket, server_chain->server);

    if (basefd == -1) goto failed;

    char* buffer = epoll_buffer_alloc();

    if (buffer == NULL) goto failed;

    epoll_event_t events[EPOLL_MAX_EVENTS];

    int timeout = -1;

    while(1) {
        int n = epoll_wait(basefd, events, EPOLL_MAX_EVENTS, timeout);

        while (--n >= 0) {
            epoll_event_t* ev = &events[n];

            socket_t* listen_socket = socket_find(ev->data.fd, basefd, (socket_t*)first_socket);

            if (listen_socket != NULL) {
                connection_t* connection = NULL;

                while ((connection = connection_create(listen_socket->fd, basefd)) != NULL) {
                    if (epoll_after_create_connection(connection, server_chain, listen_socket->server) == -1) break;
                }

                continue;
            }

            connection_t* connection = (connection_t*)ev->data.ptr;

            if (connection_trylock(connection) != 0) {
                continue;
            }

            if ((ev->events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) ||
                (server_chain->is_deprecated && server_chain->is_hard_reload)) {
                connection->close(connection);
            }
            else if (ev->events & EPOLLIN) {
                connection->read(connection, buffer, EPOLL_BUFFER);
            }
            else if (ev->events & EPOLLOUT) {
                connection->write(connection);
            }

            connection_unlock(connection);
        }

        if (server_chain->is_deprecated) {
            epoll_disable((socket_t*)first_socket, basefd);

            socket_free((socket_t*)first_socket);

            first_socket = NULL;

            timeout = 500;

            if (server_chain->connection_count == 0) break;
        }
    }

    result = 0;

    free(buffer);

    failed:

    if (result == -1) {
        socket_free((socket_t*)first_socket);
    }

    pthread_mutex_lock(&server_chain->mutex);

    server_chain->thread_count--;

    pthread_mutex_unlock(&server_chain->mutex);

    close(basefd);
}

int epoll_init(socket_epoll_t** first_socket, server_t* first_server) {
    int basefd = epoll_create1(0);

    if (basefd == -1) {
        log_error("Epoll error: Epoll create1 failed\n");
        return -1;
    }

    socket_epoll_t* last_socket = NULL;

    for (server_t* server = first_server; server; server = server->next) {
        socket_epoll_t* socket = (socket_epoll_t*)socket_listen_create(basefd, server, server->ip, server->port, (void*(*)())epoll_socket_alloc);

        if (socket == NULL) return -1;

        if (last_socket) {
            last_socket->base.next = (socket_t*)socket;
        }

        if (*first_socket == NULL) {
            *first_socket = socket;
        }

        last_socket = socket;

        socket->event->data.fd = socket->base.fd;
        socket->event->events = EPOLLIN;

        if (epoll_ctl(basefd, EPOLL_CTL_ADD, socket->base.fd, socket->event) == -1) {
            log_error("Epoll error: Epoll_ctl failed in addListener\n");
            return -1;
        }
    }

    return basefd;
}

int epoll_disable(socket_t* first_socket, int basefd) {
    socket_t* socket = first_socket;

    while (socket) {
        socket_t* next = socket->next;

        if (epoll_ctl(basefd, EPOLL_CTL_DEL, socket->fd, NULL) == -1) {
            log_error("Epoll error: disable failed\n");
        }

        socket = next;
    }
}

char* epoll_buffer_alloc() {
    return (char*)malloc(EPOLL_BUFFER);
}

int epoll_after_create_connection(connection_t* connection, server_chain_t* server_chain, server_t* server) {
    if (epoll_connection_set_event(connection) == -1) {
        log_error("Epoll error: Error event allocation\n");
        return -1;
    }

    connection->counter = &server_chain->connection_count;
    connection->server = server;

    if (server->openssl) {
        protmgr_set_tls(connection);
    } else {
        protmgr_set_http1(connection);
    }

    epoll_connection_set_hooks(connection);

    if (epoll_control_add(connection, EPOLLIN) == -1) {
        log_error("Epoll error: Error epoll_ctl failed accept\n");
        return -1;
    }

    return 0;
}

socket_epoll_t* epoll_socket_alloc() {
    socket_epoll_t* socket = (socket_epoll_t*)malloc(sizeof(socket_epoll_t));

    if (socket == NULL) return NULL;

    socket->base.fd = 0;
    socket->base.server = NULL;
    socket->base.next = NULL;
    socket->event = (epoll_event_t*)malloc(sizeof(epoll_event_t));

    return socket;
}

int epoll_after_read_request(connection_t* connection) {
    if (epoll_control_mod(connection, EPOLLOUT) == -1) {
        log_error("Epoll error: Epoll_ctl failed in read done, %d, %d\n", gettid(), errno);
        return -1;
    }

    return 0;
}

int epoll_after_write_request(connection_t* connection) {
    if (connection->keepalive_enabled == 0) {
        connection->close(connection);
    } else {
        if (epoll_control_mod(connection, EPOLLIN) == -1) {
            log_error("Epoll error: Epoll_ctl failed in write done, %d, %d\n", gettid(), errno);
            return -1;
        }
    }

    return 0;
}

int epoll_connection_close(connection_t* connection) {
    if (epoll_control_del(connection) == -1) return -1;

    SSL_shutdown(connection->ssl);
    
    SSL_clear(connection->ssl);

    shutdown(connection->fd, 2);

    close(connection->fd);

    connection_free(connection);

    return 0;
}

int epoll_control(connection_t* connection, int action, uint32_t flags) {
    void* event = connection->apidata;

    ((epoll_event_t*)connection->apidata)->events = flags;

    if (action == EPOLL_CTL_DEL) event = NULL;

    if (epoll_ctl(connection->basefd, action, connection->fd, event) == -1) {
        log_error("Epoll error: Epoll_ctl failed, %d, %d\n", gettid(), errno);
        return -1;
    }

    return 0;
}

int epoll_control_add(connection_t* connection, uint32_t flags) {
    int result = epoll_control(connection, EPOLL_CTL_ADD, flags);

    if (result == 0) {
        (*connection->counter)++;
    }

    return result;
}

int epoll_control_mod(connection_t* connection, uint32_t flags) {
    return epoll_control(connection, EPOLL_CTL_MOD, flags);
}

int epoll_control_del(connection_t* connection) {
    int result = epoll_control(connection, EPOLL_CTL_DEL, 0);

    if (result == 0) {
        (*connection->counter)--;
    }

    return result;
}

int epoll_connection_set_event(connection_t* connection) {
    epoll_event_t* event = (epoll_event_t*)malloc(sizeof(epoll_event_t));

    if (event == NULL) return -1;

    event->data.ptr = connection;

    connection->apidata = event;

    return 0;
}

void epoll_connection_set_hooks(connection_t* connection) {
    connection->close = epoll_connection_close;
    connection->after_read_request = epoll_after_read_request;
    connection->after_write_request = epoll_after_write_request;
}