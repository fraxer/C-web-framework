#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "log.h"
#include "openssl.h"
#include "route.h"
#include "http1request.h"
#include "http1response.h"
#include "http1parser.h"

#include "http1internal.h"

void http1_handle(connection_t*);
int http1_write_head(connection_t*);
int http1_write_body(connection_t*, char*, size_t, size_t);
int http1_get_resource(connection_t*);
int http1_get_file(connection_t*);
int http1_get_redirect(connection_t*);
int http1_apply_redirect(connection_t*);
char* http1_get_fullpath(connection_t*);
void http1_prepare_range(http1response_t*, ssize_t*, ssize_t*, size_t*, const size_t);


void http1_read(connection_t* connection, char* buffer, size_t buffer_size) {
    http1parser_t parser;
    http1parser_init(&parser, connection, buffer);

    while (1) {
        int bytes_readed = http1_read_internal(connection, buffer, buffer_size);

        switch (bytes_readed) {
        case -1:
            if (http1parser_payload_loaded(&parser)) {
                http1parser_free(&parser);
                http1_handle(connection);
                return;
            }
            break;
        case 0:
            http1parser_free(&parser);
            connection->keepalive_enabled = 0;
            connection->after_read_request(connection);
            return;
        default:
            http1parser_set_bytes_readed(&parser, bytes_readed);

            switch (http1parser_run(&parser)) {
            case HTTP1PARSER_ERROR:
            case HTTP1PARSER_OUT_OF_MEMORY:
                http1parser_free(&parser);
                http1response_default((http1response_t*)connection->response, 500);
                connection->after_read_request(connection);
                return;
            case HTTP1PARSER_PAYLOAD_LARGE:
                http1parser_free(&parser);
                http1response_default((http1response_t*)connection->response, 413);
                connection->after_read_request(connection);
                return;
            case HTTP1PARSER_BAD_REQUEST:
                http1parser_free(&parser);
                http1response_default((http1response_t*)connection->response, 400);
                connection->after_read_request(connection);
                return;
            case HTTP1PARSER_HOST_NOT_FOUND:
                http1parser_free(&parser);
                http1response_default((http1response_t*)connection->response, 404);
                connection->after_read_request(connection);
                return;
            case HTTP1PARSER_SUCCESS:
            case HTTP1PARSER_CONTINUE:
                break;
            }
        }
    }
}

void http1_write(connection_t* connection, char* buffer, size_t buffer_size) {
    http1response_t* response = (http1response_t*)connection->response;

    if (http1_write_head(connection) == -1) goto write;

    if (response->body.size == 0 && response->file_.size == 0) {
        http1_write_body(connection, buffer, 0, 0);
        goto write;
    }

    // body
    if (response->body.pos < response->body.size) {
        size_t payload_size = response->body.size - response->body.pos;
        ssize_t end = 0;
        ssize_t pos = response->body.pos;
        if (response->ranges) {
            http1_prepare_range(response, &pos, &end, &payload_size, response->body.size);
        }

        size_t size = payload_size > buffer_size ? buffer_size : payload_size;
        buffer = &response->body.data[pos];

        ssize_t writed = http1_write_body(connection, buffer, payload_size, size);

        if (writed < 0) goto write;

        if (response->ranges) {
            response->ranges->pos += writed;
            if (response->ranges->pos < end) return;
        }
        else {
            response->body.pos += writed;
            if (response->body.pos < response->body.size) return;
        }
    }

    // file
    if (response->file_.fd > 0 && response->file_.pos < response->file_.size) {
        size_t payload_size = response->file_.size - response->file_.pos;
        ssize_t end = 0;
        ssize_t pos = response->file_.pos;
        if (response->ranges) {
            http1_prepare_range(response, &pos, &end, &payload_size, response->file_.size);
        }

        size_t size = payload_size > buffer_size ? buffer_size : payload_size;
        lseek(response->file_.fd, pos, SEEK_SET);

        ssize_t readed = read(response->file_.fd, buffer, size);
        if (readed < 0) goto write;

        ssize_t writed = http1_write_body(connection, buffer, payload_size, readed);

        if (writed < 0) goto write;

        if (response->ranges) {
            response->ranges->pos += writed;
            if (response->ranges->pos < end) return;
        }
        else {
            response->file_.pos += writed;
            if (response->file_.pos < response->file_.size) return;
        }
    }

    write:

    connection->after_write_request(connection);
}

ssize_t http1_read_internal(connection_t* connection, char* buffer, size_t size) {
    return connection->ssl ?
        openssl_read(connection->ssl, buffer, size) :
        recv(connection->fd, buffer, size, 0);
}

ssize_t http1_write_internal(connection_t* connection, const char* response, size_t size) {
    return connection->ssl ?
        openssl_write(connection->ssl, response, size) :
        send(connection->fd, response, size, MSG_NOSIGNAL);
}

ssize_t http1_write_chunked(connection_t* connection, const char* data, size_t length, int end) {
    size_t buf_length = 10 + length + 2 + 5;
    char* buf = malloc(buf_length);

    int pos = 0;
    pos = snprintf(buf, buf_length, "%x\r\n", (unsigned int)length);
    memcpy(buf + pos, data, length); pos += length;
    memcpy(buf + pos, "\r\n", 2); pos += 2;

    if (end) {
        memcpy(buf + pos, "0\r\n\r\n", 5); pos += 5;
    }

    ssize_t writed = http1_write_internal(connection, buf, pos);

    free(buf);

    if (writed < 0) return -1;

    return length;
}

int http1_write_head(connection_t* connection) {
    http1response_t* response = (http1response_t*)connection->response;
    if (response->head_writed) return 0;

    http1response_head_t head = http1response_create_head(response);
    if (head.data == NULL) return -1;

    ssize_t writed = http1_write_internal(connection, head.data, head.size);

    free(head.data);

    response->head_writed = 1;
    
    return writed;
}

int http1_write_body(connection_t* connection, char* buffer, size_t payload_size, size_t size) {
    http1response_t* response = (http1response_t*)connection->response;
    ssize_t writed = -1;

    if (response->transfer_encoding == TE_CHUNKED) {
        if (response->content_encoding == CE_GZIP) {
            size_t source_size = size;
            int end = payload_size <= source_size;

            http1response_string_t string = response->deflate(response, buffer, size, end);
            if (string.data == NULL) goto failed;

            writed = http1_write_chunked(connection, string.data, string.size, end);

            free(string.data);

            writed = source_size;
        } else {
            writed = http1_write_chunked(connection, buffer, size, payload_size <= size);
        }
    }
    else {
        writed = http1_write_internal(connection, buffer, size);
    }

    failed:

    return writed;
}

void http1_handle(connection_t* connection) {
    http1request_t* request = (http1request_t*)connection->request;

    if (request->method == ROUTE_NONE) {
        http1response_default((http1response_t*)connection->response, 400);
        connection->after_read_request(connection);
        return;
    }

    if (http1_apply_redirect(connection)) {
        connection->after_read_request(connection);
        return;
    }

    if (http1_get_resource(connection) == 0) return;

    if (!http1request_has_payload(request))
        http1_get_file(connection);
    else
        http1response_default((http1response_t*)connection->response, 400);

    connection->after_read_request(connection);
}

int http1_get_resource(connection_t* connection) {
    http1request_t* request = (http1request_t*)connection->request;

    for (route_t* route = connection->server->http.route; route; route = route->next) {
        if (route->is_primitive && route_compare_primitive(route, request->path, request->path_length)) {
            if (route->handler[request->method] == NULL) return -1;

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

            if (route->handler[request->method] == NULL) return -1;

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

int http1_apply_redirect(connection_t* connection) {
    http1request_t* request = (http1request_t*)connection->request;
    http1response_t* response = (http1response_t*)connection->response;

    switch (http1_get_redirect(connection)) {
    case REDIRECT_OUT_OF_MEMORY:
        http1response_default(response, 500);
        return 1;
    case REDIRECT_LOOP_CYCLE:
        http1response_default(response, 508);
        return 1;
    case REDIRECT_FOUND:
        http1response_redirect(response, request->uri, 301);
        return 1;
    case REDIRECT_NOT_FOUND:
    default:
        break;
    }

    return 0;
}

int http1_get_redirect(connection_t* connection) {
    http1request_t* request = (http1request_t*)connection->request;

    int loop_cycle = 1;
    int find_new_location = 0;

    redirect_t* redirect = connection->server->http.redirect;
    while (redirect) {
        if (loop_cycle >= 10) return REDIRECT_LOOP_CYCLE;

        int vector_size = redirect->params_count * 6;
        int vector[vector_size];
        int matches_count = pcre_exec(redirect->location, NULL, request->path, request->path_length, 0, 0, vector, vector_size);

        if (matches_count < 0) {
            redirect = redirect->next;
            continue;
        }

        find_new_location = 1;

        char* new_uri = redirect_get_uri(redirect, request->path, vector);
        if (new_uri == NULL) return REDIRECT_OUT_OF_MEMORY;

        if (request->uri) free((void*)request->uri);
        request->uri = NULL;

        if (request->path) free((void*)request->path);
        request->path = NULL;

        if (request->ext) free((void*)request->ext);
        request->ext = NULL;

        if (http1response_redirect_is_external(new_uri)) {
            request->uri = new_uri;
            connection->keepalive_enabled = 0;
            return REDIRECT_FOUND;
        }

        if (http1parser_set_uri(request, new_uri, strlen(new_uri)) == -1)
            return REDIRECT_OUT_OF_MEMORY;

        redirect = connection->server->http.redirect;

        loop_cycle++;
    }

    return find_new_location ? REDIRECT_FOUND : REDIRECT_NOT_FOUND;
}

void http1_prepare_range(http1response_t* response, ssize_t* pos, ssize_t* end, size_t* payload_size, const size_t size) {
    *end = response->ranges->end > (ssize_t)size ? (ssize_t)size : response->ranges->end;
    if (response->ranges->end == -1) *end = size;

    ssize_t start = response->ranges->start == -1 ? (ssize_t)size - *end : response->ranges->start;
    if (response->ranges->pos < start) response->ranges->pos = start;

    *pos = response->ranges->pos;
    *payload_size = *end + 1 - response->ranges->pos;
}
