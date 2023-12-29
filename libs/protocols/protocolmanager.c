#include "connection.h"
#include "http1request.h"
#include "http1response.h"
#include "websocketsrequest.h"
#include "websocketsresponse.h"
#include "websocketsprotocoldefault.h"
#include "websocketsprotocolresource.h"

#include "protocolmanager.h"

void protmgr_set_tls(connection_t* connection) {
    connection->read = tls_read;
    connection->write = tls_write;
}

void protmgr_set_http1(connection_t* connection) {
    connection->read = http1_wrap_read;
    connection->write = http1_wrap_write;

    if (connection->request != NULL) {
        connection->request->free(connection->request);
    }

    connection->request = (request_t*)http1request_create(connection);

    if (connection->response != NULL) {
        connection->response->free(connection->response);
    }

    connection->response = (response_t*)http1response_create(connection);
}

void protmgr_set_websockets_default(connection_t* connection) {
    connection->read = websockets_wrap_read;
    connection->write = websockets_wrap_write;

    if (connection->request != NULL) {
        connection->request->free(connection->request);
    }

    websocketsrequest_t* request = websocketsrequest_create(connection);
    connection->request = (request_t*)request;
    request->protocol = websockets_protocol_default_create();

    if (connection->response != NULL) {
        connection->response->free(connection->response);
    }

    connection->response = (response_t*)websocketsresponse_create(connection);
}

void protmgr_set_websockets_resource(connection_t* connection) {
    connection->read = websockets_wrap_read;
    connection->write = websockets_wrap_write;

    if (connection->request != NULL) {
        connection->request->free(connection->request);
    }

    websocketsrequest_t* request = websocketsrequest_create(connection);
    connection->request = (request_t*)request;
    request->protocol = websockets_protocol_resource_create();

    if (connection->response != NULL) {
        connection->response->free(connection->response);
    }

    connection->response = (response_t*)websocketsresponse_create(connection);
}