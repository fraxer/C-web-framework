#include "../connection/connection.h"
#include "../request/http1request.h"
#include "../response/http1response.h"
#include "../request/websocketsrequest.h"
#include "../response/websocketsresponse.h"
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

    if (connection->request != NULL) {
        connection->request->free(connection->request);
    }

    connection->request = (request_t*)http1request_create(connection);

    if (connection->response != NULL) {
        connection->response->free(connection->response);
    }

    connection->response = (response_t*)http1response_create(connection);
}

void protmgr_set_websockets(connection_t* connection) {
    connection->read = websockets_read;
    connection->write = websockets_write;

    if (connection->request != NULL) {
        connection->request->free(connection->request);
    }

    connection->request = (request_t*)websocketsrequest_create(connection);

    if (connection->response != NULL) {
        connection->response->free(connection->response);
    }

    connection->response = (response_t*)websocketsresponse_create(connection);
}