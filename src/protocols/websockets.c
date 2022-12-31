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

void websockets_handle(connection_t*, websocketsparser_t*);
int websockets_get_resource(connection_t*);
int websockets_get_file(connection_t*);


void websockets_read(connection_t* connection, char* buffer, size_t buffer_size) {
    websocketsparser_t parser;

    websocketsparser_init(&parser, (websocketsrequest_t*)connection->request, buffer);

    while (1) {
        int bytes_readed = websockets_read_internal(connection, buffer, buffer_size);

        switch (bytes_readed) {
        case -1:

            // printf("fin: %d\n", parser.frame.fin);
            // printf("rsv1: %d\n", parser.frame.rsv1);
            // printf("rsv2: %d\n", parser.frame.rsv2);
            // printf("rsv3: %d\n", parser.frame.rsv3);
            // printf("opcode: %d\n", parser.frame.opcode);
            // printf("masked: %d\n", parser.frame.masked);
            // printf("payload_length: %ld\n", parser.frame.payload_length);
            // printf("mask: %d %d %d %d\n", parser.frame.mask[0], parser.frame.mask[1], parser.frame.mask[2], parser.frame.mask[3]);

            websockets_handle(connection, &parser);

            // websocketsparser_free(&parser);
            if (parser.string) free(parser.string);

            return;
        case 0:
            connection->keepalive_enabled = 0;
            connection->after_read_request(connection);
            return;
        default:
            websocketsparser_set_bytes_readed(&parser, bytes_readed);

            // printf("byte: %d\n", (unsigned char)buffer[0]);

            if (websocketsparser_run(&parser) == -1) {
                // websocketsresponse_default_response((websocketsresponse_t*)connection->response, 400);
                connection->after_read_request(connection);
                return;
            }
        }
    }
}

void websockets_write(connection_t* connection, char* buffer, size_t buffer_size) {
    websocketsresponse_t* response = (websocketsresponse_t*)connection->response;

    if (response->body.data == NULL) {
        connection->after_write_request(connection);
        return;
    }

    // body
    if (response->body.pos < response->body.size) {
        size_t size = response->body.size - response->body.pos;

        if (size > buffer_size) {
            size = buffer_size;
        }

        size_t writed = websockets_write_internal(connection, &response->body.data[response->body.pos], size);

        // printf("writed %s\n", response->body.data);

        // unsigned char c = response->body.data[1];

        // printf("%d %d %d %d %d %d %d %d\n"
        // , (c >> 7) & 0x01
        // , (c >> 6) & 0x01
        // , (c >> 5) & 0x01
        // , (c >> 4) & 0x01
        // , (c >> 3) & 0x01
        // , (c >> 2) & 0x01
        // , (c >> 1) & 0x01
        // , (c >> 0) & 0x01);

        if (writed == -1) return;

        response->body.pos += writed;

        if (writed == buffer_size) return;
    }

    // file
    // if (response->file_.fd > 0 && response->file_.pos < response->file_.size) {
    //     lseek(response->file_.fd, response->file_.pos, SEEK_SET);

    //     size_t size = response->file_.size - response->file_.pos;

    //     if (size > buffer_size) {
    //         size = buffer_size;
    //     }

    //     size_t readed = read(response->file_.fd, buffer, size);

    //     size_t writed = websockets_write_internal(connection, buffer, readed);

    //     if (writed == -1) return;

    //     response->file_.pos += writed;

    //     if (response->file_.pos < response->file_.size) return;
    // }

    connection->after_write_request(connection);





    // const char* response = "HTTP/1.1 200 OK\r\nServer: TEST\r\nContent-Length: 8\r\nContent-Type: text/html\r\nConnection: Closed\r\n\r\nResponse";

    // int size = strlen(response);

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


    // size_t writed = websockets_write_internal(connection, response, size);

    // connection->after_write_request(connection);
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

void websockets_handle(connection_t* connection, websocketsparser_t* parser) {
    websocketsrequest_t* request = (websocketsrequest_t*)connection->request;
    websocketsresponse_t* response = (websocketsresponse_t*)connection->response;

    if (parser->frame.fin == 1 && parser->frame.opcode == 10) {
        websocketsresponse_pong(response, parser->string, parser->string_len);
        connection->after_read_request(connection);
        return;
    }

    if (parser->frame.fin == 1 && parser->frame.opcode == 8) {
        websocketsresponse_close(response, parser->string, parser->string_len);
        connection->keepalive_enabled = 0;
        connection->after_read_request(connection);
        return;
    }

    if (websocketsrequest_save_payload(request, parser->string, parser->string_len) == -1) {
        connection->keepalive_enabled = 0;
        connection->after_read_request(connection);
        return;
    }

    parser->string = NULL;

    if (parser->frame.fin == 0) return;

    if (websockets_get_resource(connection) == 0) return;

    // websockets_get_file(connection);

    connection->after_read_request(connection);
}

int websockets_get_resource(connection_t* connection) {
    websocketsrequest_t* request = (websocketsrequest_t*)connection->request;
    websocketsresponse_t* response = (websocketsresponse_t*)connection->response;

    // printf("resource %s %d\n", request->payload, request->payload_length);

    response->frame_code = request->type;

    response->datan(response, request->payload, request->payload_length);

    connection->after_read_request(connection);

    return 0;

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
