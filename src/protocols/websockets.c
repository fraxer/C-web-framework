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
            websockets_handle(connection, &parser);
            return;
        case 0:
            connection->keepalive_enabled = 0;
            connection->after_read_request(connection);
            return;
        default:
            websocketsparser_set_bytes_readed(&parser, bytes_readed);

            if (websocketsparser_run(&parser) == -1) {
                websocketsparser_free(&parser);
                websocketsresponse_default_response((websocketsresponse_t*)connection->response, "bad request");
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

        size_t writed = websockets_write_internal(connection, buffer, readed);

        if (writed == -1) return;

        response->file_.pos += writed;

        if (response->file_.pos < response->file_.size) return;
    }

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

void websockets_handle(connection_t* connection, websocketsparser_t* parser) {
    websocketsrequest_t* request = (websocketsrequest_t*)connection->request;
    websocketsresponse_t* response = (websocketsresponse_t*)connection->response;

    if (parser->frame.fin == 0) return;

    if (parser->frame.fin == 1) {
        if (websocketsparser_save_location(parser, request) == -1) {
            websocketsresponse_default_response(response, "bad request");
            connection->after_read_request(connection);
            return;
        }

        if (parser->frame.opcode == 8) {
            websocketsresponse_close(response, request->control_payload, request->control_payload_length);
            connection->keepalive_enabled = 0;
            connection->after_read_request(connection);
            return;
        }
        else if (parser->frame.opcode == 9) {
            websocketsresponse_pong(response, request->control_payload, request->control_payload_length);
            connection->after_read_request(connection);
            return;
        }
        else if (parser->frame.opcode == 10) {
            websocketsrequest_reset(request);
            return;
        }
    }

    if (websockets_get_resource(connection) == 0) return;

    if (websockets_get_file(connection) == -1) {
        websocketsresponse_default_response(response, "resource not found");
    }

    connection->after_read_request(connection);
}

int websockets_get_resource(connection_t* connection) {
    websocketsrequest_t* request = (websocketsrequest_t*)connection->request;
    websocketsresponse_t* response = (websocketsresponse_t*)connection->response;

    if (request->method == ROUTE_NONE) return -1;
    if (request->path == NULL) return -1;

    for (route_t* route = connection->server->websockets.route; route; route = route->next) {
        if (route->is_primitive && route_compare_primitive(route, request->path, request->path_length)) {
            connection->handle = route->handler[request->method];
            connection->queue_push(connection);
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

                websockets_query_t* query = websockets_query_create(param->string, param->string_len, &request->path[vector[i * 2]], substring_length);

                if (query == NULL || query->key == NULL || query->value == NULL) return -1;

                websocketsparser_append_query(request, query);
            }

            websockets_query_t* query = request->query;

            while (query) {
                printf("%s -> %s\n", query->key, query->value);

                query = query->next;
            }

            connection->handle = route->handler[request->method];
            connection->queue_push(connection);
            return 0;
        }
    }

    return -1;
}

int websockets_get_file(connection_t* connection) {
    websocketsrequest_t* request = (websocketsrequest_t*)connection->request;
    websocketsresponse_t* response = (websocketsresponse_t*)connection->response;

    if (request->method == ROUTE_NONE) return -1;
    if (request->path == NULL) return -1;
    if (request->ext == NULL) return -1;

    return response->filen(response, request->path, request->path_length);
}
