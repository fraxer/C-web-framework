#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "../log/log.h"
#include "../openssl/openssl.h"
#include "../route/route.h"
#include "../request/websocketsrequest.h"
#include "../response/websocketsresponse.h"
#include "websocketsparser.h"
#include "websockets.h"
    #include <stdio.h>

void websockets_handle(connection_t*);
int websockets_get_resource(connection_t*);
int websockets_get_file(connection_t*);


void websockets_read(connection_t* connection, char* buffer, size_t buffer_size) {
    websocketsparser_t parser;

    websocketsparser_init(&parser, connection, buffer);

    while (1) {
        int bytes_readed = websockets_read_internal(connection, buffer, buffer_size);

        switch (bytes_readed) {
        case -1:
            websockets_handle(connection);
            return;
        case 0:
            connection->keepalive_enabled = 0;
            connection->after_read_request(connection);
            return;
        default:
            websocketsparser_set_bytes_readed(&parser, bytes_readed);

            // printf("byte: %d\n", (unsigned char)buffer[0]);

            if (websocketsparser_run(&parser) == -1) {
                websocketsresponse_default_response((websocketsresponse_t*)connection->response, 400);
                connection->after_read_request(connection);
                return;
            }
        }
    }
}

void websockets_write(connection_t* connection, char*, size_t) {
    const char* response = "HTTP/1.1 200 OK\r\nServer: TEST\r\nContent-Length: 8\r\nContent-Type: text/html\r\nConnection: Closed\r\n\r\nResponse";

    int size = strlen(response);


    // int max_fragment_size = 14;
    // unsigned char* buffer = (unsigned char*)malloc(size + max_fragment_size);
    
    // buffer[pos++] = frame_code;

    // if (size <= 125) {
    //     buffer[pos++] = size;
    // }
    // else if (size <= 65535) {
    //     buffer[pos++] = 126; //16 bit length follows
        
    //     buffer[pos++] = (size >> 8) & 0xFF; // leftmost first
    //     buffer[pos++] = size & 0xFF;
    // }
    // else { // >2^16-1 (65535)
    //     buffer[pos++] = 127; //64 bit length follows
        
    //     // write 8 bytes length (significant first)
        
    //     // since msg_length is int it can be no longer than 4 bytes = 2^32-1
    //     // padd zeroes for the first 4 bytes
    //     for(int i=3; i>=0; i--) {
    //         buffer[pos++] = 0;
    //     }
    //     // write the actual 32bit msg_length in the next 4 bytes
    //     for(int i=3; i>=0; i--) {
    //         buffer[pos++] = ((size >> 8*i) & 0xFF);
    //     }
    // }

    // memcpy(buffer + pos, msg, size);

    // printf("%s\n", buffer);
    // printf("%d\n", pos);
    // printf("%d\n", size);
    // printf("%d\n", size + pos);


    size_t writed = websockets_write_internal(connection, response, size);

    connection->after_write_request(connection);
}

ssize_t websockets_read_internal(connection_t* connection, char* buffer, size_t size) {
    return connection->ssl_enabled ?
        openssl_read(connection->ssl, buffer, size) :
        read(connection->fd, buffer, size);
}

ssize_t websockets_write_internal(connection_t* connection, const char* response, size_t size) {
    if (connection->ssl_enabled) {
        size_t sended = openssl_write(connection->ssl, response, size);

        if (sended == -1) {
            int err = openssl_get_status(connection->ssl, sended);
        }

        return sended;
    }
        
    return write(connection->fd, response, size);
}

void websockets_handle(connection_t* connection) {
    if (websockets_get_resource(connection) == 0) return;

    websockets_get_file(connection);

    connection->after_read_request(connection);
}

int websockets_get_resource(connection_t* connection) {
    websocketsrequest_t* request = (websocketsrequest_t*)connection->request;

    // websockets_get_redirect(connection);

    // for (route_t* route = connection->server->route; route; route = route->next) {
    //     if (route->is_primitive && route_compare_primitive(route, request->path, request->path_length)) {
    //         connection->handle = route->method[request->method];
    //         connection->queue_push(connection);
    //         return 0;
    //     }

    //     int vector_size = route->params_count * 6;
    //     int vector[vector_size];

    //     // find resource by template
    //     int matches_count = pcre_exec(route->location, NULL, request->path, request->path_length, 0, 0, vector, vector_size);

    //     if (matches_count > 1) {
    //         int i = 1; // escape full string match

    //         for (route_param_t* param = route->param; param; param = param->next, i++) {
    //             size_t substring_length = vector[i * 2 + 1] - vector[i * 2];

    //             websockets_query_t* query = websockets_query_create(param->string, param->string_len, &request->path[vector[i * 2]], substring_length);

    //             if (query == NULL || query->key == NULL || query->value == NULL) return -1;

    //             websockets_parser_append_query(request, query);
    //         }

    //         connection->handle = route->method[request->method];
    //         connection->queue_push(connection);
    //         return 0;
    //     }
    // }

    return -1;
}

int websockets_get_file(connection_t* connection) {
    websocketsrequest_t* request = (websocketsrequest_t*)connection->request;
    websocketsresponse_t* response = (websocketsresponse_t*)connection->response;

    return response->filen(response, request->path, request->path_length);
}



void websockets_parse(connection_t* connection, unsigned char* buffer, size_t buffer_size) {
    // // pong
    // if (fin == 1 && opcode == 10) {
    //     return;
    // }

    // // continuation frame
    // // if (fin == 0 && opcode == 1) {
    // //     return;
    // // }

    // {
    //     pos = 0;
    //     unsigned char frame_code = (unsigned char)0x81; // text frame

    //     char* msg = (char*)malloc(7);

    //     strcpy(msg, "Hello!");

    //     int size = strlen(msg);

    //     {
    //         char* p = (char*)realloc(msg, payload_length + 1);

    //         if (!p) {
    //             if (msg) free(msg);
    //             return;
    //         }

    //         msg = p;

    //         memcpy(msg, out_buffer + 2, payload_length - 2);

    //         if (num == 2) {
    //             frame_code = (unsigned char)0x82;
    //         }

    //         size = payload_length - 2;

    //         // if (rand() % 2 == 0) {
    //         //     sleep(1);
    //         // }
    //     }

    //     // close connection
    //     if (fin == 1 && opcode == 8) {
    //         frame_code = (unsigned char)0x88;

    //         connection->keepalive_enabled = 0;

    //         char* p = (char*)realloc(msg, payload_length + 1);

    //         if (!p) {
    //             if (msg) free(msg);
    //             return;
    //         }

    //         msg = p;

    //         memcpy(msg, out_buffer, payload_length);

    //         size = strlen(msg);
    //     }

    //     // ping
    //     if (fin == 1 && opcode == 9) {
    //         frame_code = (unsigned char)0x8A;

    //         char* p = (char*)realloc(msg, payload_length + 1);

    //         if (!p) {
    //             if (msg) free(msg);
    //             return;
    //         }

    //         msg = p;

    //         memcpy(msg, out_buffer, payload_length);

    //         size = strlen(msg);
    //     }

    //     // last frame in queue
    //     if (fin == 1 && opcode == 0) {}
    // }
}
