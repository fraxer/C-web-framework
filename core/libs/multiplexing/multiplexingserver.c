#include "log.h"
#include "socket.h"
#include "broadcast.h"
#include "multiplexing.h"
#include "multiplexingserver.h"
#include "listener.h"

static int __mpxserver_open_listener_sockets(mpxapi_t* api, server_t* server);
static mpxlistener_t* __mpxserver_listener_create(connection_t* connection);
static void __mpxserver_listener_append(mpxlistener_t*, mpxapi_t* api);
static int __mpxserver_socket_exist(socket_t* socket, server_t* server);
static void __mpxserver_close_listener_sockets(mpxapi_t* api);
static void __mpxserver_listeners_connection_close(mpxlistener_t* listener);

int mpxserver_run(appconfig_t* appconfig, void(*thread_worker_threads_pause)(appconfig_t* config)) {
    int result = 0;
    mpxapi_t* api = mpx_create(MULTIPLEX_TYPE_EPOLL);
    if (api == NULL) return result;

    if (!__mpxserver_open_listener_sockets(api, appconfig->server_chain->server))
        goto failed;

    while (1) {
        thread_worker_threads_pause(appconfig);

        api->process_events(appconfig, api);

        if (atomic_load(&appconfig->shutdown)) {
            __mpxserver_listeners_connection_close(api->listeners);

            if (api->connection_count == 0)
                break;
        }
    }

    __mpxserver_close_listener_sockets(api);

    result = 1;

    failed:

    api->free(api);

    return result;
}

void __mpxserver_listeners_connection_close(mpxlistener_t* listener) {
    if (listener->connection->destroyed) return;

    while (listener != NULL) {
        mpxlistener_t* next = listener->next;
        listener->connection->close(listener->connection);
        listener = next;
    }
}

int __mpxserver_connection_close(connection_t* connection) {
    connection_lock(connection);

    if (!connection->destroyed) {
        if (!connection->api->control_del(connection))
            log_error("Connection not removed from api\n");

        connection->destroyed = 1;

        shutdown(connection->fd, SHUT_RDWR);
    }

    connection_unlock(connection);

    return 1;
}

int __mpxserver_connection_destroy(connection_t* connection) {
    connection_lock(connection);

    if (connection->ssl != NULL) {
        SSL_shutdown(connection->ssl);
        SSL_clear(connection->ssl);
    }

    close(connection->fd);

    connection_dec(connection);

    return 1;
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
        connection->close = __mpxserver_connection_close;

        if (!api->control_add(connection, MPXIN | MPXRDHUP))
            goto failed;

        mpxlistener_t* listener = __mpxserver_listener_create(connection);
        if (listener == NULL)
            goto failed;

        __mpxserver_listener_append(listener, api);
    }

    result = 1;

    failed:

    if (!result)
        close_sockets = 1;

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

        __mpxserver_connection_destroy(listener->connection);

        mpxlistener_free(listener);

        listener = next;
    }

    api->listeners = NULL;
}
