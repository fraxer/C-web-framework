#include <stdlib.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <fcntl.h>

#include "log.h"
#include "socket.h"

int __socket_open_listen(in_addr_t, unsigned short int);
int __socket_set_options(int);

socket_t* socket_alloc() {
    socket_t* socket = malloc(sizeof(socket_t));
    if (socket == NULL) return NULL;

    socket->fd = 0;
    socket->ip = 0;
    socket->port = 0;
    socket->next = NULL;

    return socket;
}

socket_t* socket_listen_create(in_addr_t ip, unsigned short int port) {
    socket_t* result = NULL;
    socket_t* socket = (socket_t*)socket_alloc();
    if (socket == NULL) return NULL;

    socket->fd = __socket_open_listen(ip, port);
    if (socket->fd == -1) {
        log_error("Socket error: Can't create socket on port %d\n", port);
        goto failed;
    }

    result = socket;

    failed:

    if (result == NULL) {
        close(socket->fd);
        free(socket);
    }

    return result;
}

void socket_free(socket_t* socket, int closefd) {
    while (socket) {
        socket_t* next = socket->next;

        if (closefd)
            close(socket->fd);

        free(socket);

        socket = next;
    }
}

int socket_set_nonblocking(int socket) {
    int flags = fcntl(socket, F_GETFL, 0);
    if (flags == -1) {
        log_error("Socket error: fcntl failed (F_GETFL)\n");
        return -1;
    }

    flags |= O_NONBLOCK;
    if (fcntl(socket, F_SETFL, flags) == -1) {
        log_error("Socket error: fcntl failed (F_SETFL)\n");
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
    int keepidle = 5;
    if (setsockopt(socket, SOL_TCP, TCP_KEEPIDLE, &keepidle, sizeof(keepidle)) == -1) return -1;

    // Время (в секундах) между отдельными зондами
    int keepintvl = 5;
    if (setsockopt(socket, SOL_TCP, TCP_KEEPINTVL, &keepintvl, sizeof(keepintvl)) == -1) return -1;

    return 0;
}

int __socket_open_listen(in_addr_t ip, unsigned short int port) {
    struct sockaddr_in sa = {0};
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = ip;

    const int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd == -1) {
        log_error("Socket error: Can't create socket\n");
        return -1;
    }

    int result = -1;
    if (__socket_set_options(fd) == -1) {
        log_error("Socket error: Can't set oprions for socket\n");
        goto failed;
    }

    if (bind(fd, (struct sockaddr*)&sa, sizeof(sa)) == -1) {
        log_error("Socket error: Bind error\n");
        goto failed;
    }

    if (socket_set_nonblocking(fd) == -1) {
        log_error("Socket error: Can't make socket nonblocking on port %d\n", port);
        goto failed;
    }

    if (listen(fd, SOMAXCONN) == -1) {
        log_error("Socket error: listen socket failed on port %d\n", port);
        goto failed;
    }

    result = fd;

    failed:

    if (result == -1)
        close(fd);

    return result;
}

int __socket_set_options(int socket) {
    int nodelay = 1;
    if (setsockopt(socket, SOL_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay)) == -1) return -1;

    int enableaddr = 1;
    if (setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &enableaddr, sizeof(enableaddr)) == -1) return -1;

    int enableport = 1;
    if (setsockopt(socket, SOL_SOCKET, SO_REUSEPORT, &enableport, sizeof(enableport)) == -1) return -1;

    int enablecpu = 1;
    if (setsockopt(socket, SOL_SOCKET, SO_INCOMING_CPU, &enablecpu, sizeof(enablecpu)) == -1) return -1;

    return 0;
}