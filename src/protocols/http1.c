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
int http1_get_redirect(connection_t*);
char* http1_get_fullpath(connection_t*);


void http1_read(connection_t* connection, char* buffer, size_t buffer_size) {
    http1parser_t parser;

    http1parser_init(&parser, connection, buffer);

    while (1) {
        int bytes_readed = http1_read_internal(connection, buffer, buffer_size);

        switch (bytes_readed) {
        case -1:
            http1_handle(connection);
            return;
        case 0:
            connection->keepalive_enabled = 0;
            connection->after_read_request(connection);
            return;
        default:
            http1parser_set_bytes_readed(&parser, bytes_readed);

            if (http1parser_run(&parser) == -1) {
                http1parser_free(&parser);
                http1response_default_response((http1response_t*)connection->response, 400);
                connection->after_read_request(connection);
                return;
            }
        }
    }
}

void http1_write(connection_t* connection, char* buffer, size_t buffer_size) {
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
    http1request_t* request = (http1request_t*)connection->request;

    if (request->method == ROUTE_NONE) {
        http1response_default_response((http1response_t*)connection->response, 400);
        connection->after_read_request(connection);
        return;
    }

    switch (http1_get_redirect(connection)) {
    case REDIRECT_OUT_OF_MEMORY:
        http1response_default_response((http1response_t*)connection->response, 500);
        connection->after_read_request(connection);
        return;
    case REDIRECT_LOOP_CYCLE:
        http1response_default_response((http1response_t*)connection->response, 508);
        connection->after_read_request(connection);
        return;
    case REDIRECT_FOUND:
        http1response_redirect((http1response_t*)connection->response, request->uri, 301);
        connection->after_read_request(connection);
        return;
    case REDIRECT_NOT_FOUND:
    default:
        break;
    }

    if (http1_get_resource(connection) == 0) return;

    http1_get_file(connection);

    connection->after_read_request(connection);
}

int http1_get_resource(connection_t* connection) {
    http1request_t* request = (http1request_t*)connection->request;

    for (route_t* route = connection->server->http.route; route; route = route->next) {
        if (route->is_primitive && route_compare_primitive(route, request->path, request->path_length)) {
            connection->handle = route->handler[request->method];
            connection->queue_push(connection);
            // route->handler[request->method](connection->request, connection->response);
            // connection->after_read_request(connection);
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

                http1parser_append_query(request, query);
            }

            connection->handle = route->handler[request->method];
            connection->queue_push(connection);

            // route->handler[request->method](connection->request, connection->response);
            // connection->after_read_request(connection);

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

int http1_get_redirect(connection_t* connection) {
    http1request_t* request = (http1request_t*)connection->request;

    redirect_t* redirect = connection->server->http.redirect;

    int loop_cycle = 1;
    int find_new_location = 0;

    while (redirect) {
        if (loop_cycle >= 10) return REDIRECT_LOOP_CYCLE;

        int vector_size = redirect->params_count * 6;
        int vector[vector_size];

        // find resource by template
        int matches_count = pcre_exec(redirect->location, NULL, request->path, request->path_length, 0, 0, vector, vector_size);

        if (matches_count < 0) {
            redirect = redirect->next;
            continue;
        }

        find_new_location = 1;

        const char* old_uri = request->uri;
        const char* old_path = request->path;
        const char* old_ext = request->ext;

        char* new_uri = redirect_get_uri(redirect, request->path, request->path_length, vector);

        if (new_uri == NULL) return REDIRECT_OUT_OF_MEMORY;

        // if (redirect_is_external(new_uri)) {
        //     return REDIRECT_ALIAS;
        // }

        int result = http1parser_set_uri(request, new_uri, strlen(new_uri));

        if (result == -1) {
            if (old_uri != request->uri) {
                free((void*)request->uri);
                request->uri = old_uri;
            }
            if (old_path != request->path) {
                free((void*)request->path);
                request->path = old_path;
            }
            if (old_ext != request->ext) {
                free((void*)request->ext);
                request->ext = old_ext;
            }
            return REDIRECT_OUT_OF_MEMORY;
        }
        else {
            if (old_uri != request->uri) free((void*)old_uri);
            if (old_path != request->path) free((void*)old_path);
            if (old_ext != request->ext) free((void*)old_ext);
        }

        redirect = connection->server->http.redirect;

        loop_cycle++;
    }

    return find_new_location ? REDIRECT_FOUND : REDIRECT_NOT_FOUND;
}