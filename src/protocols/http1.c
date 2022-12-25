#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "../log/log.h"
#include "../openssl/openssl.h"
#include "../route/route.h"
#include "../request/http1request.h"
#include "../response/http1response.h"
#include "http1parser.h"
#include "http1.h"
    #include <stdio.h>

void http1_handle(connection_t*);
int http1_get_resource(connection_t*);
int http1_get_file(connection_t*);
void http1_get_redirect(connection_t*);
char* http1_get_fullpath(connection_t*);


void http1_read(connection_t* connection, char* buffer, size_t buffer_size) {
    http1_parser_t parser;

    http1_parser_init(&parser, connection, buffer);

    // set default handler

    while (1) {
        int bytes_readed = http1_read_internal(connection, buffer, buffer_size);

        switch (bytes_readed) {
        case -1:
            http1_handle(connection);
            // connection->after_read_request(connection);
            return;
        case 0:
            connection->keepalive_enabled = 0;
            connection->after_read_request(connection);
            return;
        default:
            http1_parser_set_bytes_readed(&parser, bytes_readed);

            if (http1_parser_run(&parser) == -1) {
                http1response_default_response((http1response_t*)connection->response, 400);
                connection->after_read_request(connection);
                return;
            }
        }
    }
}

void http1_write(connection_t* connection, char* buffer, size_t buffer_size) {

    // const char* response = "HTTP/1.1 200 OK\r\nServer: TEST\r\nContent-Length: 8\r\nContent-Type: text/html\r\nConnection: Close\r\n\r\nResponse";

    // int size = strlen(response);

    // size_t writed = http1_write_internal(connection, response, size);

    // connection->after_write_request(connection);

    http1response_t* response = (http1response_t*)connection->response;

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

        size_t writed = http1_write_internal(connection, &response->body.data[response->body.pos], size);

        if (writed == -1) return;

        response->body.pos += writed;

        if (writed == buffer_size) return;
    }

    // file
    if (response->file_.fd > 0 && response->file_.pos < response->file_.size) {
        lseek(response->file_.fd, response->file_.pos, SEEK_SET);

        size_t size = response->file_.size - response->file_.pos;

        if (size > buffer_size) {
            size = buffer_size;
        }

        size_t readed = read(response->file_.fd, buffer, size);

        size_t writed = http1_write_internal(connection, buffer, readed);

        if (writed == -1) return;

        response->file_.pos += writed;

        if (response->file_.pos < response->file_.size) return;
    }

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

            // printf("%d\n", err);
        }

        return sended;
    }
        
    return write(connection->fd, response, size);
}

void http1_handle(connection_t* connection) {
    if (http1_get_resource(connection) == 0) return;

    http1_get_file(connection);

    connection->after_read_request(connection);
}

int http1_get_resource(connection_t* connection) {
    http1request_t* request = (http1request_t*)connection->request;

    http1_get_redirect(connection);

    for (route_t* route = connection->server->route; route; route = route->next) {
        if (route->is_primitive && route_compare_primitive(route, request->path, request->path_length)) {
            connection->handle = route->method[request->method];
            connection->queue_push(connection);
            // route->method[request->method]((char*)request->path);
            return 0;
        }

        int vector_size = route->params_count * 6;
        int vector[vector_size];

        // find resource by template
        int matches_count = pcre_exec(route->location, NULL, request->path, request->path_length, 0, 0, vector, vector_size);

        if (matches_count > 1) {
            int i = 1; // escape full string match

            for (route_param_t* param = route->param; param; param = param->next, i++) {
                size_t substring_length = vector[i * 2 + 1] - vector[i * 2];

                http1_query_t* query = http1_query_create(param->string, param->string_len, &request->path[vector[i * 2]], substring_length);

                if (query == NULL || query->key == NULL || query->value == NULL) return -1;

                http1_parser_append_query(request, query);
            }

            connection->handle = route->method[request->method];
            connection->queue_push(connection);

            // route->method[request->method]((char*)request->path);

            return 0;
        }
    }

    return -1;
}

int http1_get_file(connection_t* connection) {
    http1request_t* request = (http1request_t*)connection->request;
    http1response_t* response = (http1response_t*)connection->response;

    return response->filen(response, request->path, request->path_length);
}

void http1_get_redirect(connection_t* connection) {
    http1request_t* request = (http1request_t*)connection->request;

    redirect_t* redirect = connection->server->redirect;

    for (; redirect; redirect = redirect->next) {
        
    }
}