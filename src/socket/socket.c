#include <stddef.h>
#include <stdlib.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "../log/log.h"
#include "socket.h"
    #include <stdio.h>

static socket_t* first_socket = NULL;
static socket_t* last_socket = NULL;

int socket_bind(in_addr_t, unsigned short int);

int socket_set_nonblocking(int);

int socket_accept_connection(socket_t*, int);

int socket_set_keepalive(int);

socket_t* socket_alloc() {
    socket_t* socket = (socket_t*)malloc(sizeof(socket_t));

    if (socket == NULL) return NULL;

    socket->port = 0;
    socket->fd = 0;
    socket->next = NULL;

    return socket;
}

int socket_create(int epollfd, in_addr_t ip, unsigned short int port) {
    socket_t* socket = socket_alloc();

    if (socket == NULL) return -1;

    socket->fd = socket_bind(ip, port);
    socket->port = port;

    if (first_socket == NULL) {
        first_socket = socket;
    }

    if (last_socket) {
        last_socket->next = socket;
    }

    last_socket = socket;

    printf("socket: %d\n", socket->fd);

    if (socket->fd == -1) {
        log_error("Socket error: Can't create socket on port %d\n", port);
        return -1;
    }

    if (socket_set_nonblocking(socket->fd) == -1) {
        log_error("Socket error: Can't make socket nonblocking on port %d\n", port);
        return -1;
    }

    if (listen(socket->fd, SOMAXCONN) == -1) {
        log_error("Socket error: listen socket failed on port %d\n", port);
        return -1;
    }

    socket->event.data.fd = socket->fd;
    socket->event.events = EPOLLIN | EPOLLEXCLUSIVE;

    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, socket->fd, &socket->event) == -1) {
        log_error("Socket error: Epoll_ctl failed in addListener\n");
        return -1;
    }

    return 0;
}

void socket_free() {
    socket_t* socket = first_socket;

    while (socket) {
        socket_t* next = socket->next;

        close(socket->fd);

        free(socket);

        socket = next;
    }
}

void socket_reset_internal() {

}

int socket_bind(in_addr_t ip, unsigned short int port) {

    struct sockaddr_in sa = {0};

    sa.sin_family      = AF_INET;
    sa.sin_port        = htons(port);
    sa.sin_addr.s_addr = ip;

    int socketfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (socketfd == -1) {
        log_error("[ERROR][server.cpp][create_and_bind] Socket error\n");
        return -1;
    }

    int enableaddr = 1;
    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &enableaddr, sizeof(enableaddr)) == -1) return -1;

    int enableport = 1;
    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEPORT, &enableport, sizeof(enableport)) == -1) return -1;

    int enablecpu = 1;
    if (setsockopt(socketfd, SOL_SOCKET, SO_INCOMING_CPU, &enablecpu, sizeof(enablecpu)) == -1) return -1;

    int defer_accept = 1;
    if (setsockopt(socketfd, SOL_SOCKET, TCP_DEFER_ACCEPT, &defer_accept, sizeof(defer_accept)) == -1) return -1;

    if (bind(socketfd, (struct sockaddr*)&sa, sizeof(sa)) == -1) {
        log_error("Socketerror: Bind error\n");
        return -1;
    }

    return socketfd;
}

int socket_set_nonblocking(int socket) {

    int flags = fcntl(socket, F_GETFL, 0);

    if (flags == -1) {
        log_error("Socket error: fcntl failed (F_GETFL)\n");
        return -1;
    }

    flags |= O_NONBLOCK;
    // flags |= O_ASYNC;

    int s = fcntl(socket, F_SETFL, flags);

    // fcntl(socket, F_SETOWN, gettid());

    if (s == -1) {
        log_error("Socket error: fcntl failed (F_SETFL)\n");
        return -1;
    }

    return 0;
}

int socket_is_current(epoll_event_t* ev, int epollfd) {
    for (socket_t* socket = first_socket; socket; socket = socket->next) {
        if (socket->fd == ev->data.fd) {
            return socket_accept_connection(socket, epollfd);
        }
    }

    return -1;
}

int socket_accept_connection(socket_t* socket, int epollfd) {
    struct sockaddr in_addr;

    socklen_t in_len = sizeof(in_addr);

    int infd = accept(socket->fd, &in_addr, &in_len);

    if (infd == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) { // Done processing incoming connections 
            log_error("Socket error: Error accept EAGAIN\n");
            return -1;
        }
        else {
            log_error("Socket error: Error accept failed\n");
            return -1;
        }
    }

    struct sockaddr_in addr;

    socklen_t addr_size = sizeof(struct sockaddr_in);

    if (getpeername(infd, (struct sockaddr *)&addr, &addr_size) == -1) {
        log_error("Socket error: Error getpeername\n");
        return -1;
    }

    // cout << inet_ntoa(addr.sin_addr) << ", " << addr.sin_addr.s_addr << ", " << inet_addr("109.194.35.137") << endl;

    if (socket_set_keepalive(infd) == -1) {
        log_error("Socket error: Error set keepalive\n");
        return -1;
    }

    if (socket_set_nonblocking(infd) == -1) {
        log_error("Socket error: Error make_socket_nonblocking failed\n");
        return -1;
    }

    // obj* p_obj;

    // pthread_mutex_lock(&lock_objs);
    // unordered_map<int, obj*>::iterator it = objs->find(infd);
    // if (it != objs->end()) {
    //     pthread_mutex_unlock(&lock_objs);
    //     p_obj = (it->second);
    // } else {
    //     p_obj = objs->emplace(infd, new obj()).first->second;
    //     pthread_mutex_unlock(&lock_objs);
    // }
    // p_obj->port      = socket->port;
    // p_obj->ip        = (char*)inet_ntoa(addr.sin_addr);
    // p_obj->in_thread = false;
    // p_obj->handler   = nullptr;
    // p_obj->boundary  = nullptr;
    // p_obj->boundary_half = nullptr;
    // // printf("port %d\n", p_obj->port);

    socket->event.data.fd = infd;
    socket->event.events = EPOLLIN | EPOLLEXCLUSIVE;



    // if(addr.sin_addr.s_addr == inet_addr("109.194.35.137")) {
    //     socket->event.events   = EPOLLOUT;
    //     p_obj->status = 403;
    // }

    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, infd, &socket->event) == -1) {
        log_error("Socket error: Error epoll_ctl failed accept\n");
        return -1;
    }

    return 0;
}

int socket_set_keepalive(int socket) {
    int kenable = 1;
    if (setsockopt(socket, SOL_SOCKET, SO_KEEPALIVE, &kenable, sizeof(kenable)) == -1) return -1;

    // Максимальное количество контрольных зондов
    int keepcnt = 3;
    if (setsockopt(socket, SOL_TCP, TCP_KEEPCNT, &keepcnt, sizeof(keepcnt)) == -1) return -1;

    // Время (в секундах) соединение должно оставаться бездействующим
    // до того, как TCP начнет отправлять контрольные зонды
    int keepidle = 40;
    if (setsockopt(socket, SOL_TCP, TCP_KEEPIDLE, &keepidle, sizeof(keepidle)) == -1) return -1;

    // Время (в секундах) между отдельными зондами
    int keepintvl = 5;
    if (setsockopt(socket, SOL_TCP, TCP_KEEPINTVL, &keepintvl, sizeof(keepintvl)) == -1) return -1;

    return 0;
}