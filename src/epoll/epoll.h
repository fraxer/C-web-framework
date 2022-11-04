#ifndef __EPOLL__
#define __EPOLL__

#define EPOLL_MAX_EVENTS 16

typedef struct epoll_event epoll_event_t;

void epoll_run();

#endif