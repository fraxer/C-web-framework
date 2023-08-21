#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#include "log.h"
#include "openssl.h"
#include "route.h"
#include "websocketsrequest.h"
#include "websocketsresponse.h"
#include "websocketsparser.h"
#include "websocketsinternal.h"

void websockets_handle(connection_t*, websocketsparser_t*);
int websockets_get_resource(connection_t*);
int websockets_get_file(connection_t*);


void websockets_read(connection_t* connection, char* buffer, size_t buffer_size) {
    websocketsparser_t* parser = ((websocketsrequest_t*)connection->request)->parser;
    websocketsparser_set_connection(parser, connection);
    websocketsparser_set_buffer(parser, buffer);

    while (1) {
        int bytes_readed = websockets_read_internal(connection, buffer, buffer_size);

        switch (bytes_readed) {
        case -1:
            return;
        case 0:
            connection->keepalive_enabled = 0;
            connection->after_read_request(connection);
            return;
        default:
            websocketsparser_set_bytes_readed(parser, bytes_readed);

            switch (websocketsparser_run(parser)) {
            case WSPARSER_ERROR:
            case WSPARSER_OUT_OF_MEMORY:
                websocketsresponse_default_response((websocketsresponse_t*)connection->response, "server error");
                connection->after_read_request(connection);
                return;
            case WSPARSER_PAYLOAD_LARGE:
                websocketsresponse_default_response((websocketsresponse_t*)connection->response, "payload large");
                connection->after_read_request(connection);
                return;
            case WSPARSER_BAD_REQUEST:
                websocketsresponse_default_response((websocketsresponse_t*)connection->response, "bad request");
                connection->after_read_request(connection);
                return;
            case WSPARSER_CONTINUE:
                break;
            case WSPARSER_COMPLETE:
                websockets_handle(connection, parser);
                websocketsparser_reset(parser);
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

        if (size > buffer_size)
            size = buffer_size;

        ssize_t writed = websockets_write_internal(connection, &response->body.data[response->body.pos], size);

        if (writed == -1) return;

        response->body.pos += writed;

        if ((size_t)writed == buffer_size) return;
    }

    // file
    if (response->file_.fd > 0 && response->file_.pos < response->file_.size) {
        lseek(response->file_.fd, response->file_.pos, SEEK_SET);

        size_t size = response->file_.size - response->file_.pos;

        if (size > buffer_size)
            size = buffer_size;

        size_t readed = read(response->file_.fd, buffer, size);

        ssize_t writed = websockets_write_internal(connection, buffer, readed);

        if (writed == -1) return;

        response->file_.pos += writed;

        if (response->file_.pos < response->file_.size) return;
    }

    connection->after_write_request(connection);
}

ssize_t websockets_read_internal(connection_t* connection, char* buffer, size_t size) {
    return connection->ssl ?
        openssl_read(connection->ssl, buffer, size) :
        read(connection->fd, buffer, size);
}

ssize_t websockets_write_internal(connection_t* connection, const char* response, size_t size) {
    if (connection->ssl)
        return openssl_write(connection->ssl, response, size);

    return write(connection->fd, response, size);
}

void websockets_handle(connection_t* connection, websocketsparser_t* parser) {
    websocketsresponse_t* response = (websocketsresponse_t*)connection->response;
    websocketsrequest_t* request = (websocketsrequest_t*)connection->request;

    if (parser->frame.fin) {
        if (parser->frame.opcode == WSOPCODE_CLOSE) {
            websocketsresponse_close(response, bufferdata_get(&parser->buf), bufferdata_writed(&parser->buf));
            connection->keepalive_enabled = 0;
            connection->after_read_request(connection);
            return;
        }
        else if (parser->frame.opcode == WSOPCODE_PING) {
            websocketsresponse_pong(response, bufferdata_get(&parser->buf), bufferdata_writed(&parser->buf));
            connection->after_read_request(connection);
            return;
        }
        else if (parser->frame.opcode == WSOPCODE_PONG) {
            return;
        }
    }
    else return;

    if (request->protocol->get_resource(connection)) return;

    websocketsresponse_default_response(response, "resource not found");

    connection->after_read_request(connection);
}
