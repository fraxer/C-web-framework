#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>

#include "http1request.h"
#include "http1response.h"
#include "cookieparser.h"
#include "domain.h"
#include "log.h"
#include "appconfig.h"
#include "http1common.h"
#include "http1responseparser.h"
#include "helpers.h"

int __http1responseparser_set_protocol(http1response_t*, http1responseparser_t*);
int __http1responseparser_set_statuscode(http1response_t*, http1responseparser_t*);
int __http1responseparser_set_header_key(http1response_t*, http1responseparser_t*);
int __http1responseparser_set_header_value(http1response_t*, http1responseparser_t*);
int __http1responseparser_parse_payload(http1responseparser_t*);
void __http1responseparser_try_set_keepalive(http1responseparser_t*);
void __http1responseparser_try_set_content_length(http1responseparser_t*);
void __http1responseparser_try_set_transfer_encoding(http1responseparser_t*);
void __http1responseparser_try_set_content_encoding(http1responseparser_t*);
void __http1responseparser_flush(http1responseparser_t*);
int __http1responseparser_transfer_decoding(http1response_t*, const char*, size_t);


void http1responseparser_init(http1responseparser_t* parser) {
    parser->stage = HTTP1RESPONSEPARSER_PROTOCOL;
    parser->bytes_readed = 0;
    parser->pos_start = 0;
    parser->pos = 0;
    parser->buffer = NULL;
    parser->connection = NULL;
    parser->content_length = 0;
    parser->content_saved_length = 0;

    parser->teparser = http1teparser_init();
    if (parser->teparser == NULL) return;

    bufferdata_init(&parser->buf);
}

void http1responseparser_set_connection(http1responseparser_t* parser, connection_t* connection) {
    parser->connection = connection;
    parser->teparser->connection = connection;
}

void http1responseparser_set_buffer(http1responseparser_t* parser, char* buffer) {
    parser->buffer = buffer;
}

void http1responseparser_free(http1responseparser_t* parser) {
    __http1responseparser_flush(parser);
    free(parser);
}

void http1responseparser_reset(http1responseparser_t* parser) {
    __http1responseparser_flush(parser);
    http1responseparser_init(parser);
}

void __http1responseparser_flush(http1responseparser_t* parser) {
    if (parser->buf.dynamic_buffer) free(parser->buf.dynamic_buffer);
    parser->buf.dynamic_buffer = NULL;

    http1teparser_free(parser->teparser);
}

int http1responseparser_run(http1responseparser_t* parser) {
    http1response_t* response = (http1response_t*)parser->connection->response;
    http1request_t* request = (http1request_t*)parser->connection->request;
    parser->pos_start = 0;
    parser->pos = 0;

    if (parser->stage == HTTP1RESPONSEPARSER_PAYLOAD)
        return __http1responseparser_parse_payload(parser);

    for (parser->pos = parser->pos_start; parser->pos < parser->bytes_readed; parser->pos++) {
        char ch = parser->buffer[parser->pos];

        switch (parser->stage)
        {
        case HTTP1RESPONSEPARSER_PROTOCOL:
            if (ch == ' ') {
                parser->stage = HTTP1RESPONSEPARSER_STATUS_CODE;

                bufferdata_complete(&parser->buf);
                if (!__http1responseparser_set_protocol(response, parser))
                    return HTTP1PARSER_BAD_REQUEST;

                bufferdata_reset(&parser->buf);
                break;
            }
            else {
                if (bufferdata_writed(&parser->buf) > 8)
                    return HTTP1PARSER_BAD_REQUEST;

                bufferdata_push(&parser->buf, ch);
                break;
            }
        case HTTP1RESPONSEPARSER_STATUS_CODE:
            if (ch == ' ') {
                parser->stage = HTTP1RESPONSEPARSER_STATUS_TEXT;

                bufferdata_complete(&parser->buf);
                if (!__http1responseparser_set_statuscode(response, parser))
                    return HTTP1PARSER_BAD_REQUEST;

                bufferdata_reset(&parser->buf);
                break;
            }
            else if (http1parser_is_ctl(ch)) {
                return HTTP1PARSER_BAD_REQUEST;
            }
            else {
                bufferdata_push(&parser->buf, ch);
                break;
            }
        case HTTP1RESPONSEPARSER_STATUS_TEXT:
            if (ch == '\r') {
                parser->stage = HTTP1RESPONSEPARSER_NEWLINE1;

                bufferdata_complete(&parser->buf);
                bufferdata_reset(&parser->buf);
            }
            else {
                if (bufferdata_writed(&parser->buf) > 128)
                    return HTTP1PARSER_BAD_REQUEST;

                bufferdata_push(&parser->buf, ch);
            }
            break;
        case HTTP1RESPONSEPARSER_NEWLINE1:
            if (ch == '\n') {
                parser->stage = HTTP1RESPONSEPARSER_HEADER_KEY;
                break;
            }
            else
                return HTTP1PARSER_BAD_REQUEST;
        case HTTP1RESPONSEPARSER_HEADER_KEY:
            if (ch == '\r') {
                if (bufferdata_writed(&parser->buf) > 0)
                    return HTTP1PARSER_BAD_REQUEST;

                parser->stage = HTTP1RESPONSEPARSER_NEWLINE3;
                break;
            }
            else if (ch == ':') {
                parser->stage = HTTP1RESPONSEPARSER_HEADER_SPACE;

                bufferdata_complete(&parser->buf);
                if (!__http1responseparser_set_header_key(response, parser))
                    return HTTP1PARSER_BAD_REQUEST;

                bufferdata_reset(&parser->buf);

                break;
            }
            else if (http1parser_is_ctl(ch)) {
                return HTTP1PARSER_BAD_REQUEST;
            }
            else {
                bufferdata_push(&parser->buf, ch);
                break;
            }
        case HTTP1RESPONSEPARSER_HEADER_SPACE:
            if (ch == ' ') {
                parser->stage = HTTP1RESPONSEPARSER_HEADER_VALUE;
                break;
            }
            else
                return HTTP1PARSER_BAD_REQUEST;
        case HTTP1RESPONSEPARSER_HEADER_VALUE:
            if (ch == '\r') {
                parser->stage = HTTP1RESPONSEPARSER_NEWLINE2;

                bufferdata_complete(&parser->buf);
                if (!__http1responseparser_set_header_value(response, parser))
                    return HTTP1PARSER_BAD_REQUEST;

                bufferdata_reset(&parser->buf);

                break;
            }
            else if (http1parser_is_ctl(ch))
                return HTTP1PARSER_BAD_REQUEST;
            else
            {
                bufferdata_push(&parser->buf, ch);
                break;
            }
        case HTTP1RESPONSEPARSER_NEWLINE2:
            if (ch == '\n') {
                parser->stage = HTTP1RESPONSEPARSER_HEADER_KEY;
                break;
            }
            else
                return HTTP1PARSER_BAD_REQUEST;
        case HTTP1RESPONSEPARSER_NEWLINE3:
            if (ch == '\n') {
                parser->stage = HTTP1RESPONSEPARSER_PAYLOAD;

                if (response->transfer_encoding == TE_NONE && parser->content_length == 0)
                    return HTTP1RESPONSEPARSER_COMPLETE;
                if (request->method == ROUTE_HEAD)
                    return HTTP1RESPONSEPARSER_COMPLETE;

                break;
            }
            else
                return HTTP1PARSER_BAD_REQUEST;
        case HTTP1RESPONSEPARSER_PAYLOAD:
            return __http1responseparser_parse_payload(parser);
        default:
            return HTTP1PARSER_BAD_REQUEST;
        }
    }

    return HTTP1PARSER_CONTINUE;
}

void http1responseparser_set_bytes_readed(http1responseparser_t* parser, int readed) {
    parser->bytes_readed = readed;
}

int __http1responseparser_set_protocol(http1response_t* response, http1responseparser_t* parser) {
    char* string = bufferdata_get(&parser->buf);

    if (string[0] == 'H' && string[1] == 'T' && string[2] == 'T' && string[3] == 'P' && string[4] == '/'  && string[5] == '1' && string[6] == '.' && string[7] == '1') {
        response->version = HTTP1_VER_1_1;
        return 1;
    }

    return 0;
}

int __http1responseparser_set_statuscode(http1response_t* response, http1responseparser_t* parser) {
    char* string = bufferdata_get(&parser->buf);
    int status_code = atoi(string);
    if (status_code == 0)
        return 0;

    response->status_code = status_code;

    return 1;
}

int __http1responseparser_parse_payload(http1responseparser_t* parser) {
    http1response_t* response = (http1response_t*)parser->connection->response;

    if (!http1response_has_payload(response))
        return HTTP1PARSER_BAD_REQUEST;

    parser->pos_start = parser->pos;
    parser->pos = parser->bytes_readed;

    if (response->payload_.file.fd <= 0) {
        response->payload_.path = create_tmppath(env()->main.tmp);
        if (response->payload_.path == NULL)
            return HTTP1PARSER_ERROR;

        response->payload_.file.fd = mkstemp(response->payload_.path);
        if (response->payload_.file.fd == -1)
            return HTTP1PARSER_ERROR;
    }

    const size_t string_len = parser->pos - parser->pos_start;
    const size_t client_max_body_size = env()->main.client_max_body_size;

    if (parser->content_saved_length + string_len > client_max_body_size)
        return HTTP1PARSER_PAYLOAD_LARGE;

    if (response->transfer_encoding == TE_CHUNKED) {
        int ret = __http1responseparser_transfer_decoding(response, &parser->buffer[parser->pos_start], string_len);
        switch (ret) {
            case HTTP1TEPARSER_ERROR:
                return HTTP1PARSER_ERROR;
            case HTTP1TEPARSER_CONTINUE:
                break;
            case HTTP1TEPARSER_COMPLETE:
                return HTTP1RESPONSEPARSER_COMPLETE;
        }
    }
    else {
        if (response->content_encoding == CE_GZIP) {
            gzip_t* const gzip = &parser->connection->gzip;
            char buffer[GZIP_BUFFER];

            if (!gzip_inflate_init(gzip, &parser->buffer[parser->pos_start], string_len))
                return HTTP1PARSER_ERROR;

            do {
                const size_t writed = gzip_inflate(gzip, buffer, GZIP_BUFFER);
                if (gzip_inflate_has_error(gzip))
                    return HTTP1PARSER_ERROR;

                if (writed > 0)
                    response->payload_.file.append_content(&response->payload_.file, buffer, writed);
            } while (gzip_want_continue(gzip));

            if (gzip_is_end(gzip))
                if (!gzip_inflate_free(gzip))
                    return HTTP1PARSER_ERROR;
        }
        else {
            if (!response->payload_.file.append_content(&response->payload_.file, &parser->buffer[parser->pos_start], string_len))
                return HTTP1PARSER_ERROR;
        }

        parser->content_saved_length += string_len;
        if (parser->content_saved_length >= parser->content_length)
            return HTTP1RESPONSEPARSER_COMPLETE;
    }

    return HTTP1PARSER_CONTINUE;
}

void __http1responseparser_try_set_keepalive(http1responseparser_t* parser) {
    http1response_t* response = (http1response_t*)parser->connection->response;
        http1_header_t* header = response->last_header;

    const char* connection_key = "connection";
    ssize_t connection_key_length = 10;
    const char* connection_value = "keep-alive";
    ssize_t connection_value_length = 10;

    if ((ssize_t)header->key_length != connection_key_length) return;
    if (!cmpstrn_lower(header->key, header->key_length, connection_key, connection_key_length)) return;

    parser->connection->keepalive_enabled = 0;
    if (cmpstrn_lower(header->value, header->value_length, connection_value, connection_value_length)) {
        parser->connection->keepalive_enabled = 1;
    }
}

void __http1responseparser_try_set_content_length(http1responseparser_t* parser) {
    http1response_t* response = (http1response_t*)parser->connection->response;
    http1_header_t* header = response->last_header;

    const char* key = "content-length";
    const size_t key_length = 14;

    if (header->key_length != key_length) return;
    if (!cmpstrn_lower(header->key, header->key_length, key, key_length)) return;

    parser->content_length = atoll(header->value);
    response->content_length = parser->content_length;
}


void __http1responseparser_try_set_transfer_encoding(http1responseparser_t* parser) {
    http1response_t* response = (http1response_t*)parser->connection->response;
    http1_header_t* header = response->last_header;

    const char* key = "transfer-encoding";
    const size_t key_length = 17;

    if (header->key_length != key_length) return;
    if (!cmpstrn_lower(header->key, header->key_length, key, key_length)) return;

    const char* value_chunked = "chunked";
    const size_t value_chunked_length = 7;

    const char* value_gzip = "gzip";
    const size_t value_gzip_length = 4;

    if (cmpstrn_lower(header->value, header->value_length, value_chunked, value_chunked_length))
        response->transfer_encoding = TE_CHUNKED;
    else if (cmpstrn_lower(header->value, header->value_length, value_gzip, value_gzip_length))
        response->transfer_encoding = TE_GZIP;
}

void __http1responseparser_try_set_content_encoding(http1responseparser_t* parser) {
    http1response_t* response = (http1response_t*)parser->connection->response;
    http1_header_t* header = response->last_header;

    const char* key = "content-encoding";
    const size_t key_length = 16;

    if (header->key_length != key_length) return;
    if (!cmpstrn_lower(header->key, header->key_length, key, key_length)) return;

    const char* value_gzip = "gzip";
    const size_t value_gzip_length = 4;
    if (cmpstrn_lower(header->value, header->value_length, value_gzip, value_gzip_length))
        response->content_encoding = CE_GZIP;
}

int __http1responseparser_set_header_key(http1response_t* response, http1responseparser_t* parser) {
    char* string = bufferdata_get(&parser->buf);
    size_t length = bufferdata_writed(&parser->buf);
    http1_header_t* header = http1_header_create(string, length, NULL, 0);

    if (header == NULL) {
        log_error("HTTP error: can't alloc header memory\n");
        return 0;
    }
    
    if (header->key == NULL)
        return 0;

    if (response->header_ == NULL)
        response->header_ = header;

    if (response->last_header)
        response->last_header->next = header;

    response->last_header = header;

    return 1;
}

int __http1responseparser_set_header_value(http1response_t* response, http1responseparser_t* parser) {
    char* string = bufferdata_get(&parser->buf);
    size_t length = bufferdata_writed(&parser->buf);

    response->last_header->value = http1_set_field(string, length);
    response->last_header->value_length = length;

    if (response->last_header->value == NULL)
        return HTTP1PARSER_OUT_OF_MEMORY;

    __http1responseparser_try_set_keepalive(parser);
    __http1responseparser_try_set_content_length(parser);
    __http1responseparser_try_set_transfer_encoding(parser);
    __http1responseparser_try_set_content_encoding(parser);

    return HTTP1PARSER_CONTINUE;
}

int __http1responseparser_transfer_decoding(http1response_t* response, const char* data, size_t size) {
    http1teparser_t* parser = ((http1responseparser_t*)response->parser)->teparser;
    http1teparser_set_buffer(parser, (char*)data, size);

    return http1teparser_run(parser);
}
