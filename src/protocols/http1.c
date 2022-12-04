#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "../log/log.h"
#include "../openssl/openssl.h"
#include "../request/http1request.h"
#include "http1.h"
    #include <stdio.h>

typedef enum http1_request_stage {
    SPACE = 0,
    METHOD,
    URI,
    PROTOCOL,
    HEADER_KEY,
    HEADER_VALUE,
    PAYLOAD
} http1_request_stage_e;

typedef struct http1_parser {
    http1_request_stage_e stage;
    int carriage_return;
    size_t bytes_readed;
    size_t pos_start;
    size_t pos;
    size_t string_len;
    char* string;
    char* buffer;
    connection_t* connection;
} http1_parser_t;

int http1_parser_init(http1_parser_t*, connection_t*, char*);
int http1_parser_run(http1_parser_t*);
int http1_parser_parse_method(http1_parser_t*);
int http1_parser_parse_uri(http1_parser_t*);
int http1_parser_parse_protocol(http1_parser_t*);
int http1_parser_parse_headers_key(http1_parser_t*);
int http1_parser_parse_headers_value(http1_parser_t*);
int http1_parser_parse_header_key(http1_parser_t*);
int http1_parser_parse_header_value(http1_parser_t*);
int http1_parser_parse_endhead(http1_parser_t*);
int http1_parser_parse_payload(http1_parser_t*);
int http1_set_method(http1request_t*, const char*, size_t);
int http1_parser_string_append(http1_parser_t*x);
int http1_set_uri(http1request_t*, const char*, size_t);
int http1_set_protocol(http1request_t*, const char*);
http1request_header_t* http1_header_alloc();
http1request_header_t* http1_header_create(const char*, const char*);


void http1_read(connection_t* connection, char* buffer, size_t size) {
    http1_parser_t parser;

    http1_parser_init(&parser, connection, buffer);

    while (1) {
        int bytes_readed = http1_read_internal(connection, buffer, size);

        switch (bytes_readed) {
        case -1:
            connection->after_read_request(connection);
            return;
        case 0:
            connection->after_read_request(connection);
            // connection->keepalive_enabled = 0;
            // connection->close(connection);
            return;
        default:
            parser.bytes_readed = bytes_readed;

            if (http1_parser_run(&parser) == -1) {
                connection->after_read_request(connection);
                return;
            }
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

int http1_parser_init(http1_parser_t* parser, connection_t* connection, char* buffer) {
    parser->stage = METHOD;
    parser->carriage_return = 0;
    parser->bytes_readed = 0;
    parser->pos_start = 0;
    parser->pos = 0;
    parser->string_len = 0;
    parser->string = NULL;
    parser->buffer = buffer;
    parser->connection = connection;

    return 0;
}

int http1_parser_run(http1_parser_t* parser) {
    int result = 0;

    parser->pos_start = 0;
    parser->pos = 0;

    switch (parser->stage) {
    case METHOD:
        result = http1_parser_parse_method(parser);
        if (result < 0) return result;
    case URI:
        result = http1_parser_parse_uri(parser);
        if (result < 0) return result;
    case PROTOCOL:
        result = http1_parser_parse_protocol(parser);
        if (result < 0) return result;
    case HEADER_KEY:
        result = http1_parser_parse_headers_key(parser);
        if (result < 0) return result;
    case HEADER_VALUE:
        result = http1_parser_parse_headers_value(parser);
        if (result < 0) return result;
    case PAYLOAD:
        result = http1_parser_parse_payload(parser);
        if (result < 0) return result;
    }

    return result;
}

int http1_parser_parse_method(http1_parser_t* parser) {
    for (parser->pos = parser->pos_start; parser->pos < parser->bytes_readed; parser->pos++) {
        if (parser->buffer[parser->pos] == ' ') {
            goto next;
        }
        if (parser->pos > 7) return -1;
    }

    return http1_parser_string_append(parser);

    next:

    if (parser->string && http1_parser_string_append(parser) == -1) return -1;

    http1request_t* request = (http1request_t*)parser->connection->request;

    if (parser->string) {
        if (http1_set_method(request, parser->string, parser->string_len) == -1) return -1;
    } else {
        if (http1_set_method(request, &parser->buffer[parser->pos_start], parser->pos - parser->pos_start) == -1) return -1;
    }

    parser->pos_start = parser->pos + 1;

    if (parser->string) {
        free(parser->string);
        parser->string = NULL;
    }

    parser->string_len = 0;

    parser->stage = URI;

    if (parser->pos + 1 == parser->bytes_readed) return -2;

    return 0;
}

int http1_parser_parse_uri(http1_parser_t* parser) {
    for (parser->pos = parser->pos_start; parser->pos < parser->bytes_readed; parser->pos++) {
        if (parser->buffer[parser->pos] == ' ') {
            goto next;
        }
    }

    return http1_parser_string_append(parser);

    next:

    if (parser->string && http1_parser_string_append(parser) == -1) return -1;

    http1request_t* request = (http1request_t*)parser->connection->request;

    if (parser->string) {
        if (http1_set_uri(request, parser->string, parser->string_len) == -1) return -1;
    } else {
        if (http1_set_uri(request, &parser->buffer[parser->pos_start], parser->pos - parser->pos_start) == -1) return -1;
    }

    parser->pos_start = parser->pos + 1;

    if (parser->string) {
        free(parser->string);
        parser->string = NULL;
    }

    parser->string_len = 0;

    parser->stage = PROTOCOL;

    if (parser->pos + 1 == parser->bytes_readed) return -2;

    return 0;
}

int http1_parser_parse_protocol(http1_parser_t* parser) {
    for (parser->pos = parser->pos_start; parser->pos < parser->bytes_readed; parser->pos++) {
        if (parser->buffer[parser->pos] == '\n') {
            if (parser->buffer[parser->pos - 1] == '\r') {
                parser->carriage_return = 1;
            }
            else if (parser->string && parser->string[parser->string_len - 1] == '\r') {
                parser->carriage_return = 1;
            }
            goto next;
        }
    }

    return http1_parser_string_append(parser);

    next:

    if (parser->string && http1_parser_string_append(parser) == -1) return -1;

    http1request_t* request = (http1request_t*)parser->connection->request;

    if (parser->string) {
        if (http1_set_protocol(request, parser->string) == -1) return -1;
    } else {
        if (http1_set_protocol(request, &parser->buffer[parser->pos_start]) == -1) return -1;
    }

    parser->pos_start = parser->pos + 1;

    if (parser->string) {
        free(parser->string);
        parser->string = NULL;
    }

    parser->string_len = 0;

    parser->stage = HEADER_KEY;

    parser->carriage_return = 0;

    if (parser->pos + 1 == parser->bytes_readed) return -2;

    return 0;
}

int http1_parser_parse_headers_key(http1_parser_t* parser) {
    int result = 0;

    for (parser->pos = parser->pos_start; parser->pos < parser->bytes_readed;) {
        result = http1_parser_parse_header_key(parser);
        if (result < 0) return result;

        result = http1_parser_parse_header_value(parser);
        if (result < 0) return result;
    }

    return 0;
}

int http1_parser_parse_headers_value(http1_parser_t* parser) {
    int result = 0;

    for (parser->pos = parser->pos_start; parser->pos < parser->bytes_readed;) {
        result = http1_parser_parse_header_value(parser);
        if (result < 0) return result;

        result = http1_parser_parse_header_key(parser);
        if (result < 0) return result;
    }

    return 0;
}

int http1_parser_parse_header_key(http1_parser_t* parser) {
    for (parser->pos = parser->pos_start; parser->pos < parser->bytes_readed; parser->pos++) {
        // printf("hkch %d\n", parser->buffer[parser->pos], parser->buffer[parser->pos]);

        if (parser->buffer[parser->pos] == ' ') {
            // printf("asd %.*s\n", parser->pos - parser->pos_start, &parser->buffer[parser->pos_start]);
            if (parser->buffer[parser->pos - 1] == ':') {
                parser->carriage_return = 1;
            }
            else if (parser->string && parser->string[parser->string_len - 1] == ':') {
                parser->carriage_return = 1;
            }
            goto next;
        }
        else if (parser->buffer[parser->pos] == '\n') {
            int result = http1_parser_parse_endhead(parser);

            if (result == 0) {
                return http1_parser_parse_payload(parser);
            }

            return result;
        }
    }

    return http1_parser_string_append(parser);

    next:

    if (parser->string && http1_parser_string_append(parser) == -1) return -1;

    http1request_t* request = (http1request_t*)parser->connection->request;

    const char* header_value = NULL;

    if (parser->string) {
        if (http1_set_header(request, parser->string, header_value) == -1) return -1;
    } else {
        parser->string_len = parser->pos - parser->pos_start;
        parser->string = (char*)malloc(parser->string_len + 1);

        if (parser->string == NULL) return -1;

        strncpy(parser->string, &parser->buffer[parser->pos_start], parser->string_len);

        parser->string[parser->string_len] = 0;

        if (http1_set_header(request, parser->string, header_value) == -1) return -1;
    }

    // printf("KK %s\n", parser->string);

    parser->pos_start = parser->pos + 1;

    parser->string = NULL;

    parser->string_len = 0;

    parser->stage = HEADER_VALUE;

    parser->carriage_return = 0;

    if (parser->pos + 1 == parser->bytes_readed) return -2;

    return 0;
}

int http1_parser_parse_header_value(http1_parser_t* parser) {
    // printf("hvch %c %d\n", parser->buffer[parser->pos_start], parser->buffer[parser->pos_start]);
    for (parser->pos = parser->pos_start; parser->pos < parser->bytes_readed; parser->pos++) {
        if (parser->buffer[parser->pos] == '\n') {
            if (parser->buffer[parser->pos - 1] == '\r') {
                parser->carriage_return = 1;
            }
            else if (parser->string && parser->string[parser->string_len - 1] == '\r') {
                parser->carriage_return = 1;
            }
            goto next;
        }
    }

    return http1_parser_string_append(parser);

    next:

    if (parser->string && http1_parser_string_append(parser) == -1) return -1;

    http1request_t* request = (http1request_t*)parser->connection->request;

    if (parser->string) {
        request->last_header->value = parser->string;
    } else {
        parser->string_len = parser->pos - parser->pos_start;
        parser->string = (char*)malloc(parser->string_len + 1);

        if (parser->string == NULL) return -1;

        strncpy(parser->string, &parser->buffer[parser->pos_start], parser->string_len);

        parser->string[parser->string_len] = 0;

        request->last_header->value = parser->string;
    }

    // printf("VV %s\n", parser->string);

    parser->pos_start = parser->pos + 1;

    parser->string = NULL;

    parser->string_len = 0;

    parser->stage = HEADER_KEY;

    parser->carriage_return = 0;

    if (parser->pos + 1 == parser->bytes_readed) return -2;

    return 0;
}

int http1_parser_parse_endhead(http1_parser_t* parser) {
    parser->pos_start = parser->pos + 1;

    if (parser->string) {
        free(parser->string);
        parser->string = NULL;
    }

    parser->string_len = 0;

    parser->stage = PAYLOAD;

    parser->carriage_return = 0;

    if (parser->pos + 1 == parser->bytes_readed) return -2;

    return 0;
}

int http1_parser_parse_payload(http1_parser_t* parser) {
    // printf("payload\n");

    for (parser->pos = parser->pos_start; parser->pos < parser->bytes_readed; parser->pos++) {
        // printf("%c %d\n", parser->buffer[parser->pos], parser->buffer[parser->pos]);
    }

    return -2;
}

int http1_set_method(http1request_t* request, const char* string, size_t length) {
    switch (length) {
    case 3:
        if (string[0] == 'G' && string[1] == 'E' && string[2] == 'T') {
            request->method = HTTP1_GET;
        }
        else if (string[0] == 'P' && string[1] == 'U' && string[2] == 'T') {
            request->method = HTTP1_PUT;
        }
        break;
    case 4:
        if (string[0] == 'P' && string[1] == 'O' && string[2] == 'S' && string[3] == 'T') {
            request->method = HTTP1_POST;
        }
        break;
    case 5:
        if (string[0] == 'P' && string[1] == 'A' && string[2] == 'T' && string[3] == 'C' && string[4] == 'H') {
            request->method = HTTP1_PATCH;
        }
        break;
    case 6:
        if (string[0] == 'D' && string[1] == 'E' && string[2] == 'L' && string[3] == 'E' && string[4] == 'T' && string[5] == 'E') {
            request->method = HTTP1_DELETE;
        }
        break;
    case 7:
        if (string[0] == 'O' && string[1] == 'P' && string[2] == 'T' && string[3] == 'I' && string[4] == 'O' && string[5] == 'N' && string[6] == 'S') {
            request->method = HTTP1_OPTIONS;
        }
        break;
    default:
        log_error("HTTP error: method error\n");
        return -1;
    }

    return 0;
}

int http1_parser_string_append(http1_parser_t* parser) {
    size_t string_len = (parser->pos - parser->carriage_return) - parser->pos_start;

    if (parser->string == NULL) {
        parser->string_len = string_len;

        parser->string = (char*)malloc(parser->string_len + 1);

        if (parser->string == NULL) return -1;

        strncpy(parser->string, &parser->buffer[parser->pos_start], parser->string_len);
    } else {
        size_t len = parser->string_len;
        parser->string_len = parser->string_len + string_len;

        char* data = (char*)realloc(parser->string, parser->string_len + 1);

        if (data == NULL) {
            free(parser->string);
            parser->string = NULL;
            return -1;
        }

        parser->string = data;

        if (parser->string_len > len) {
            strncat(parser->string, &parser->buffer[parser->pos_start], string_len);
        }
    }

    parser->string[parser->string_len] = 0;

    return -2;
}

int http1_set_uri(http1request_t* request, const char* string, size_t length) {
    // printf("%.*s\n", length, string);

    return 0;
}

int http1_set_protocol(http1request_t* request, const char* string) {
    if (string[0] == 'H' && string[1] == 'T' && string[2] == 'T' && string[3] == 'P' && string[4] == '/'  && string[5] == '1' && string[6] == '.' && string[7] == '1') {
        request->version = HTTP1_VER_1_1;
        return 0;
    }
    else if (string[0] == 'H' && string[1] == 'T' && string[2] == 'T' && string[3] == 'P' && string[4] == '/'  && string[5] == '1' && string[6] == '.' && string[7] == '0') {
        request->version = HTTP1_VER_1_0;
        return 0;
    }

    log_error("HTTP error: version error\n");

    return -1;
}

int http1_set_header(http1request_t* request, const char* key, const char* value) {
    // printf("SH %s - %d\n", key, strlen(key));

    http1request_header_t* header = http1_header_create(key, value);

    if (header == NULL) {
        log_error("HTTP error: version error\n");
        return -1;
    }

    if (request->header == NULL) {
        request->header = header;
    }

    if (request->last_header) {
        request->last_header->next = header;
    } else {
        request->last_header = header;
    }

    return 0;
}

http1request_header_t* http1_header_alloc() {
    return (http1request_header_t*)malloc(sizeof(http1request_header_t));
}

http1request_header_t* http1_header_create(const char* key, const char* value) {
    http1request_header_t* header = http1_header_alloc();

    header->key = NULL;
    header->value = NULL;
    header->next = NULL;

    return header;
}

// http1request_header_t* http1_get_last_header(http1request_t* request) {
//     http1request_header_t* last_header = request->header;

//     while (last_header) {
//         if (last_header->next) {
//             last_header = last_header->next;
//         }
//     }

//     return last_header;
// }