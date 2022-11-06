#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "../log/log.h"
#include "../epoll/epoll.h"
#include "http1.h"
    #include <stdio.h>

int SSL_read(void*, const void*, int);
int SSL_write(void*, const void*, int);

void http1_read(connection_t* connection, char* buffer, size_t size) {
    while (1) {
        int readed = http1_read_internal(connection, buffer, size);

        if (readed == -1) {
            connection->after_read_request(connection);
            return;
        }
        else if (readed == 0) {
            connection->after_read_request(connection);
            return;
        }
    }
}

void http1_write(connection_t* connection) {
    const char* response = "HTTP/1.1 200 OK\r\nServer: TEST\r\nContent-Length: 8\r\nContent-Type: text/html\r\nConnection: Closed\r\n\r\nResponse";

    int size = strlen(response);

    size_t writed = http1_write_internal(connection, response, size);

    connection->after_write_request(connection);
}

ssize_t http1_read_internal(connection_t* connection, char* buffer, size_t size) {
    return connection->ssl_enabled ?
        SSL_read(connection->ssl, buffer, size) :
        read(connection->fd, buffer, size);
}

ssize_t http1_write_internal(connection_t* connection, const char* response, size_t size) {
    if (connection->ssl_enabled) {
        size_t sended = SSL_write(connection->ssl, response, size);

        if (sended == -1) {
            // int err = ssl_get_error(connection->ssl, sended);

            // if (err == 1) return;
        }

        return sended;
    }
        
    return write(connection->fd, response, size);
}

int SSL_read(void*, const void*, int) { return 0; }
int SSL_write(void*, const void*, int) { return 0; }