#include <unistd.h>

#include "log.h"
#include "socket.h"
#include "multiplexing.h"
#include "multiplexingserver.h"
#include "listener.h"

int __mpxserver_open_listener_sockets(mpxapi_t*, server_t*);
mpxlistener_t* __mpxserver_listener_create(connection_t*);
void __mpxserver_listener_append(mpxlistener_t*, mpxapi_t*);
int __mpxserver_socket_exist(socket_t*, server_t*);
void __mpxserver_close_listener_sockets(mpxapi_t*);

void mpxserver_run(server_chain_t* server_chain) {
    mpxapi_t* api = mpx_create(MULTIPLEX_TYPE_EPOLL);
    if (api == NULL) return;

    api->is_deprecated = &server_chain->is_deprecated;

    if (!__mpxserver_open_listener_sockets(api, server_chain->server))
        goto failed;

    while (1) {
        api->process_events(api);

        if (server_chain->is_deprecated) {
            __mpxserver_close_listener_sockets(api);

            if (api->connection_count == 0) break;
        }
    }

    failed:

    server_chain->thread_count--;

    api->free(api);

    return;
}

int __mpxserver_open_listener_sockets(mpxapi_t* api, server_t* first_server) {
    socket_t* first_socket = NULL;
    socket_t* last_socket = NULL;
    int result = 0;
    int close_sockets = 0;
    for (server_t* server = first_server; server; server = server->next) {
        if (__mpxserver_socket_exist(first_socket, server)) continue;

        socket_t* socket = socket_listen_create(server->ip, server->port);
        if (socket == NULL) goto failed;

        if (last_socket)
            last_socket->next = socket;

        if (first_socket == NULL)
            first_socket = socket;

        last_socket = socket;

        connection_t* connection = connection_alloc(socket->fd, api, server->ip, server->port);
        if (connection == NULL) goto failed;

        connection->api = api;
        connection->read = listener_read;
        connection->server = first_server;
        connection->close = listener_connection_close;

        if (!api->control_add(connection, MPXIN | MPXRDHUP))
            goto failed;

        mpxlistener_t* listener = __mpxserver_listener_create(connection);
        if (listener == NULL)
            goto failed;

        __mpxserver_listener_append(listener, api);
    }

    result = 1;

    failed:

    close_sockets = !result;
    socket_free(first_socket, close_sockets);

    return result;
}

mpxlistener_t* __mpxserver_listener_create(connection_t* connection) {
    mpxlistener_t* listener = malloc(sizeof * listener);
    if (listener == NULL) return NULL;

    listener->connection = connection;
    listener->next = NULL;

    return listener;
}

void __mpxserver_listener_append(mpxlistener_t* listener, mpxapi_t* api) {
    if (listener == NULL) return;
    if (api == NULL) return;

    listener->next = api->listeners;

    api->listeners = listener;
}

int __mpxserver_socket_exist(socket_t* socket, server_t* server) {
    while (socket) {
        if (socket->ip == server->ip && socket->port == server->port)
            return 1;

        socket = socket->next;
    }

    return 0;
}

void __mpxserver_close_listener_sockets(mpxapi_t* api) {
    mpxlistener_t* listener = api->listeners;
    while (listener) {
        mpxlistener_t* next = listener->next;
        listener->connection->close(listener->connection);

        mpxlistener_free(listener);

        listener = next;
    }

    api->listeners = NULL;
}
