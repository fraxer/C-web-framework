#include <stddef.h>
#include <sys/epoll.h>
#include <pthread.h>
#include "epoll.h"
#include "../log/log.h"

void epoll_run() {
    int epollfd = epoll_create1(0);

    if (epollfd == -1) {
        log_error("Thread error: Epoll create1 failed\n");
        return;
    }

    epoll_event_t events[EPOLL_MAX_EVENTS];

    // vector<socket_t*>    sockets;

    // addListener(epollfd, sockets, HOST_IP, 443);
    // addListener(epollfd, sockets, HOST_IP, 80);

    // obj*   p_obj;

    // char*  buf = (char*)malloc(BUFSIZ32);

    // conn_st* db_conn = new conn_st();

    // db_conn->conn = NULL;
    // db_conn->ttl  = chrono::system_clock::now() - chrono::hours(DB_CONNECTION_TTL + 1);

    // pthread_mutex_lock(&lock_objs);
    // db_connection.emplace(gettid(), db_conn);
    // pthread_mutex_unlock(&lock_objs);

    int n;

    epoll_event_t* ev;

    // while(1) {

    //     n = epoll_wait(epollfd, events, EPOLL_MAX_EVENTS, -1);

    //     while(--n >= 0) {

    //         ev = &events[n];
    //         int fd = ev->data.fd;

    //         pthread_mutex_lock(&lock_objs);
    //         it = objs->find(fd);
    //         if (it != objs->end()) {
    //             pthread_mutex_unlock(&lock_objs);
    //             p_obj = (it->second);
    //         } else {
    //             p_obj = objs->emplace(fd, new obj()).first->second;
    //             pthread_mutex_unlock(&lock_objs);
    //         }

    //         if(p_obj->active.load(memory_order_acquire)) {
    //             // log_error("[INFO][server.cpp][thread_func] Cell already used\n");
    //             continue;
    //         }

    //         // p_obj->active.store(true, memory_order_release);

    //         bool b = false;

    //         while(!p_obj->active.compare_exchange_strong(b, true, memory_order_acquire));

    //         if(ev->events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP) ) {

    //             // printf("[ERROR][server.cpp][thread_func] EPOLLERR %d, errno %d, uri %s, active %d, in thread %d, is polling %d, bofy len %ld\n",
    //             //     fd, errno, p_obj->uri, p_obj->active.load(), p_obj->in_thread, p_obj->is_longpoll, p_obj->response_body ? strlen(p_obj->response_body) : 0);

    //             if(epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL) == -1) {
    //                 syslog(LOG_ERR, "[ERROR][server.cpp][thread_func] Epoll_ctl failed del, %d\n", errno);
    //             }

    //             p_obj->keep_alive = false;

    //             clearConnectionObject(p_obj, ev->data.fd);

    //             shutdown(fd, 2);

    //             close(fd);
    //         }
    //         else if(is_current_socket(sockets, ev, objs, epollfd)) {
    //             // printf("fd %d CONN\n", fd);
    //         }
    //         else if(ev->events & EPOLLIN) {

    //             // printf("fd %d EPOLLIN, in thread %d\n", fd, p_obj->in_thread);

    //             if(p_obj->port == 80 || p_obj->sslConnected) {
    //                 handleDataRead(p_obj, ev, epollfd, buf);
    //                 // log_error("[INFO][server.cpp][thread_func] Read\n");
    //             }
    //             else {
    //                 handleHandshake(p_obj, ev, epollfd);
    //                 // log_error("[INFO][server.cpp][thread_func] Handshake read\n");
    //             }
    //         }
    //         else if(ev->events & EPOLLOUT) {

    //             // printf("fd %d EPOLLOUT, in thread %d\n", fd, p_obj->in_thread);

    //             if(p_obj->port == 80 || p_obj->sslConnected) {
    //                 handleDataWrite(p_obj, ev, epollfd);
    //                 // log_error("[INFO][server.cpp][thread_func] Write\n");
    //             }
    //             else {
    //                 handleHandshake(p_obj, ev, epollfd);
    //                 // log_error("[INFO][server.cpp][thread_func] Handshake write\n");
    //             }
    //         }
    //         p_obj->active.store(false, memory_order_release);
    //     }
    // }

    // free(buf);

    // for (size_t i = 0; i < sockets.size(); i++) {
    //     close(sockets[i]->socket);
    //     delete sockets[i];
    // }

    close(epollfd);
}