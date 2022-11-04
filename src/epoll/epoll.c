#include <stddef.h>
#include <pthread.h>
#include <unistd.h>
#include "epoll.h"
#include "../log/log.h"
#include "../socket/socket.h"
#include "../server/server.h"
    #include <stdio.h>

static int epollfd = 0;

void epoll_run() {
    epoll_event_t events[EPOLL_MAX_EVENTS];

    while(1) {
        int n = epoll_wait(epollfd, events, EPOLL_MAX_EVENTS, -1);

        printf("events, %d\n", n);

        while(--n >= 0) {

            epoll_event_t* ev = &events[n];

            int fd = ev->data.fd;

            printf("%d\n", fd);

            if(ev->events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP) ) {

                printf("err\n");

                if (epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL) == -1) {
                    log_error("[ERROR][server.cpp][thread_func] Epoll_ctl failed del\n");
                }

                // p_obj->keep_alive = false;

                // clearConnectionObject(p_obj, ev->data.fd);

                shutdown(fd, 2);

                close(fd);
            }
            else if(socket_is_current(ev, epollfd) == 0) {
                printf("fd %d new conn\n", fd);
            }
            else if(ev->events & EPOLLIN) {

                printf("fd %d EPOLLIN\n", fd);

                // handleOnRead(ev->data->ptr);

                // if(p_obj->port == 80 || p_obj->sslConnected) {
                //     handleDataRead(p_obj, ev, epollfd, buf);
                //     // log_error("[INFO][server.cpp][thread_func] Read\n");
                // }
                // else {
                //     handleHandshake(p_obj, ev, epollfd);
                //     // log_error("[INFO][server.cpp][thread_func] Handshake read\n");
                // }
            }
            else if(ev->events & EPOLLOUT) {

                printf("out\n");

                // handleOnWrite(ev->data->ptr);

                // printf("fd %d EPOLLOUT, in thread %d\n", fd, p_obj->in_thread);

                // if(p_obj->port == 80 || p_obj->sslConnected) {
                //     handleDataWrite(p_obj, ev, epollfd);
                //     // log_error("[INFO][server.cpp][thread_func] Write\n");
                // }
                // else {
                //     handleHandshake(p_obj, ev, epollfd);
                //     // log_error("[INFO][server.cpp][thread_func] Handshake write\n");
                // }
            }
        }
    }

    socket_free();

    epoll_close();
}

int epoll_init() {
    epollfd = epoll_create1(0);

    if (epollfd == -1) {
        log_error("Epoll error: Epoll create1 failed\n");
        return -1;
    }

    for (server_t* server = server_get_first(); server; server = server->next) {
        printf("%p, %d\n", server, server->port);
        if (socket_create(epollfd, server->ip, server->port) == -1) return -1;
    }

    return 0;
}

int epoll_close() {
    return close(epollfd);
}

