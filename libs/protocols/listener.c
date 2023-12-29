#include "listener.h"
#include "log.h"
#include "multiplexing.h"
#include "protocolmanager.h"
#include "connection_queue.h"

int __listener_create_connection(connection_t*, server_t*);
void __listener_connection_set_hooks(connection_t*);
int __listener_after_read_request(connection_t*);
int __listener_after_write_request(connection_t*);
int __listener_queue_prepend(connection_queue_item_t*);
int __listener_queue_append(connection_queue_item_t*);
int __listener_queue_pop(connection_t*);
int __listener_connection_set_event(connection_t*);

void listener_read(connection_t* socket_connection, char* buffer, size_t buffer_size) {
    (void)buffer;
    (void)buffer_size;
    connection_t* connection = connection_create(socket_connection);
    if (connection == NULL) return;

    __listener_create_connection(connection, socket_connection->server);
}

int __listener_create_connection(connection_t* connection, server_t* server) {
    connection->server = server;

    while (server) {
        if (server->ip == connection->ip && server->port == connection->port) {
            connection->server = server;
            break;
        }

        server = server->next;
    }

    if (connection->server->openssl)
        protmgr_set_tls(connection);
    else
        protmgr_set_http1(connection);

    __listener_connection_set_hooks(connection);

    return connection->api->control_add(connection, MPXIN | MPXRDHUP);
}

void __listener_connection_set_hooks(connection_t* connection) {
    connection->close = listener_connection_close;
    connection->after_read_request = __listener_after_read_request;
    connection->after_write_request = __listener_after_write_request;
    connection->queue_prepend = __listener_queue_prepend;
    connection->queue_append = __listener_queue_append;
    connection->queue_pop = __listener_queue_pop;
}

int __listener_after_read_request(connection_t* connection) {
    return connection->api->control_mod(connection, MPXOUT | MPXRDHUP);
}

int __listener_after_write_request(connection_t* connection) {
    if (connection->keepalive_enabled == 0)
        return connection->close(connection);

    connection_reset(connection);

    if (connection->switch_to_protocol != NULL) {
        connection->switch_to_protocol(connection);
        connection->switch_to_protocol = NULL;
    }

    cqueue_lock(connection->queue);
    const int onwrite = !cqueue_empty(connection->queue);
    cqueue_unlock(connection->queue);

    if (onwrite) {
        atomic_store(&connection->onwrite, 1);
        return 1;
    }

    atomic_store(&connection->onwrite, 0);

    return connection->api->control_mod(connection, MPXIN | MPXRDHUP);
}

int __listener_queue_prepend(connection_queue_item_t* item) {
    if (!item->connection->api->control_mod(item->connection, MPXONESHOT))
        return 0;

    atomic_store(&item->connection->onwrite, 1);

    connection_queue_guard_prepend(item);

    return 1;
}

int __listener_queue_append(connection_queue_item_t* item) {
    connection_queue_guard_append(item);
    return 1;
}

int __listener_queue_pop(connection_t* connection) {
    return connection->api->control_mod(connection, MPXOUT | MPXRDHUP);
}

int listener_connection_close(connection_t* connection) {
    if (!connection->api->control_del(connection))
        log_error("Connection not removed from api\n");

    if (connection->ssl != NULL) {
        SSL_shutdown(connection->ssl);
        SSL_clear(connection->ssl);
    }

    shutdown(connection->fd, 2);
    close(connection->fd);
    connection->fd = 0;

    cqueue_lock(connection->queue);
    const int queue_empty = cqueue_empty(connection->queue);
    cqueue_unlock(connection->queue);

    if (queue_empty)
        connection_free(connection);

    return 1;
}
