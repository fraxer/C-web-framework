#include "../connection/connection.h"
#include "tls.h"
#include "http1.h"
#include "websockets.h"

void protmgr_set_tls(connection_t* connection) {
    connection->read = tls_read;
    connection->write = tls_write;
}

void protmgr_set_http1(connection_t* connection) {
    connection->read = http1_read;
    connection->write = http1_write;
}

void protmgr_set_websockets(connection_t* connection) {
    connection->read = websockets_read;
    connection->write = websockets_write;
}