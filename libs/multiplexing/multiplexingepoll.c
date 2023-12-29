#include <unistd.h>
#include <stdlib.h>
#include <sys/epoll.h>

#include "log.h"
#include "config.h"
#include "multiplexingepoll.h"

#define EPOLL_MAX_EVENTS 16

int __mpx_epoll_init();
epoll_config_t* __mpx_epoll_config_init(int);
void __mpx_epoll_free(void*);
int __mpx_epoll_control_add(connection_t*, int);
int __mpx_epoll_control_mod(connection_t*, int);
int __mpx_epoll_control_del(connection_t*);
int __mpx_epoll_process_events(void*);
int __mpx_epoll_convert_events(int);
int __mpx_epoll_control(connection_t*, int, uint32_t);
void __mpx_epoll_config_free(epoll_config_t*);

mpxapi_epoll_t* mpx_epoll_init() {
    mpxapi_epoll_t* result = NULL;
    mpxapi_epoll_t* api = malloc(sizeof * api);
    if (api == NULL) {
        log_error("Epoll error: Epoll api alloc failed\n");
        return result;
    }

    const int fd = __mpx_epoll_init();

    api->base.connection_count = 0;
    api->base.is_deprecated = NULL;
    api->base.config = __mpx_epoll_config_init(fd);
    api->base.listeners = NULL;
    api->base.free = __mpx_epoll_free;
    api->base.control_add = __mpx_epoll_control_add;
    api->base.control_del = __mpx_epoll_control_del;
    api->base.control_mod = __mpx_epoll_control_mod;
    api->base.process_events = __mpx_epoll_process_events;
    api->fd = fd;

    if (api->base.config == NULL) goto failed;

    result = api;

    failed:

    if (result == NULL) {
        api->base.free(api);
        close(fd);
    }

    return result;
}

int __mpx_epoll_init() {
    const int basefd = epoll_create1(0);
    if (basefd == -1) {
        log_error("Epoll error: Epoll create1 failed\n");
        return -1;
    }

    return basefd;
}

epoll_config_t* __mpx_epoll_config_init(int basefd) {
    epoll_config_t* iconfig = malloc(sizeof * iconfig);
    if (iconfig == NULL) return NULL;

    iconfig->basefd = basefd;
    iconfig->timeout = -1;
    iconfig->is_hard_reload = config()->main.reload == CONFIG_RELOAD_HARD;
    iconfig->buffer = malloc(sizeof(char) * config()->main.read_buffer);
    iconfig->buffer_size = config()->main.read_buffer;

    if (iconfig->buffer == NULL) {
        free(iconfig);
        return NULL;
    }

    return iconfig;
}

void __mpx_epoll_free(void* arg) {
    mpxapi_epoll_t* api = arg;
    if (api == NULL) return;

    if (api->fd)
        close(api->fd);

    __mpx_epoll_config_free(api->base.config);

    free(api);
}

int __mpx_epoll_control_add(connection_t* connection, int events) {
    int result = __mpx_epoll_control(connection, EPOLL_CTL_ADD, __mpx_epoll_convert_events(events));
    if (result) connection->api->connection_count++;

    return result;
}

int __mpx_epoll_control_mod(connection_t* connection, int events) {
    return __mpx_epoll_control(connection, EPOLL_CTL_MOD, __mpx_epoll_convert_events(events));
}

int __mpx_epoll_control_del(connection_t* connection) {
    int result = __mpx_epoll_control(connection, EPOLL_CTL_DEL, 0);
    if (result) connection->api->connection_count--;

    return result;
}

int __mpx_epoll_convert_events(int events) {
    uint32_t epoll_events = 0;

    if (events & MPXIN) epoll_events |= EPOLLIN;
    if (events & MPXOUT) epoll_events |= EPOLLOUT;
    if (events & MPXERR) epoll_events |= EPOLLERR;
    if (events & MPXRDHUP) epoll_events |= EPOLLRDHUP;
    if (events & MPXONESHOT) epoll_events |= EPOLLONESHOT;

    return epoll_events;
}

int __mpx_epoll_process_events(void* arg) {
    mpxapi_t* api = arg;
    epoll_config_t* apiconfig = api->config;
    epoll_event_t events[EPOLL_MAX_EVENTS];
    int n = epoll_wait(apiconfig->basefd, events, EPOLL_MAX_EVENTS, apiconfig->timeout);

    while (--n >= 0) {
        epoll_event_t* ev = &events[n];
        connection_t* connection = ev->data.ptr;

        if (connection == NULL) continue;

        if ((ev->events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) || (*api->is_deprecated && apiconfig->is_hard_reload))
            connection->close(connection);
        else if (ev->events & EPOLLIN)
            connection->read(connection, apiconfig->buffer, apiconfig->buffer_size);
        else if (ev->events & EPOLLOUT)
            connection->write(connection, apiconfig->buffer, apiconfig->buffer_size);
    }

    return 1;
}

int __mpx_epoll_control(connection_t* connection, int action, uint32_t flags) {
    mpxapi_epoll_t* api = (mpxapi_epoll_t*)connection->api;
    epoll_event_t event = {
        .data = {
            .ptr = connection
        },
        .events = flags
    };
    epoll_event_t* pevent = &event;

    if (action == EPOLL_CTL_DEL) pevent = NULL;

    if (epoll_ctl(api->fd, action, connection->fd, pevent) == -1) {
        log_error("Epoll error: Epoll_ctl failed\n");
        return 0;
    }

    return 1;
}

void __mpx_epoll_config_free(epoll_config_t* config) {
    if (config == NULL) return;

    if (config->buffer != NULL)
        free(config->buffer);

    free(config);
}

