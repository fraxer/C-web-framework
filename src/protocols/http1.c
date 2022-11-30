#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "../log/log.h"
#include "../openssl/openssl.h"
#include "http1.h"
    #include <stdio.h>

void http1_read(connection_t* connection, char* buffer, size_t size) {
    while (1) {
        int readed = http1_read_internal(connection, buffer, size);

        switch (readed) {
        case -1:
            connection->after_read_request(connection);
            return;
        case 0:
            connection->after_read_request(connection);
            // connection->keepalive_enabled = 0;
            // connection->close(connection);
            return;
        default:
            // TODO: parse request
        }
    }
}

void http1_write(connection_t* connection) {
    const char* response = "HTTP/1.1 200 OK\r\nServer: TEST\r\nContent-Length: 8\r\nContent-Type: text/html\r\nConnection: Close\r\n\r\nResponse";

    int size = strlen(response);

    size_t writed = http1_write_internal(connection, response, size);

    connection->after_write_request(connection);
}

ssize_t http1_read_internal(connection_t* connection, char* buffer, size_t size) {
    return connection->ssl_enabled ?
        openssl_read(connection->ssl, buffer, size) :
        read(connection->fd, buffer, size);
}

ssize_t http1_write_internal(connection_t* connection, const char* response, size_t size) {
    if (connection->ssl_enabled) {
        size_t sended = openssl_write(connection->ssl, response, size);

        if (sended == -1) {
            int err = openssl_get_status(connection->ssl, sended);

            if (err == 1) return 0;
        }

        return sended;
    }
        
    return write(connection->fd, response, size);
}