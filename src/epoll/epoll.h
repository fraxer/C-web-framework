#ifndef __EPOLL__
#define __EPOLL__

#include <sys/epoll.h>

#define EPOLL_MAX_EVENTS 16

typedef struct epoll_event epoll_event_t;

void epoll_run();

int epoll_init();

int epoll_close();

#endif