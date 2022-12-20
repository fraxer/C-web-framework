#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "../log/log.h"
#include "../openssl/openssl.h"
#include "../route/route.h"
#include "../mimetype/mimetype.h"
#include "../request/http1request.h"
#include "../response/http1response.h"
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
int http1_parser_string_append(http1_parser_t*);
int http1_set_uri(http1request_t*, const char*, size_t);
int http1_set_path(http1request_t*, const char*, size_t);
int http1_set_ext(http1request_t*, const char*, size_t);
int http1_set_query(http1request_t*, const char*, size_t, size_t);
void http1_append_query(http1request_t*, http1_query_t*);
int http1_set_protocol(http1request_t*, const char*);
http1_header_t* http1_header_alloc();
http1_header_t* http1_header_create(const char*, const char*);
http1_query_t* http1_query_alloc();
http1_query_t* http1_query_create();
const char* http1_query_set_field(const char*, size_t);
void http1_handle(connection_t*);
int http1_get_resource(connection_t*);
int http1_get_file(connection_t*);
const char* http1_get_mimetype(const char*);
void http1_default_response(connection_t*, int);
void http1_get_redirect(connection_t*);


void http1_read(connection_t* connection, char* buffer, size_t size) {
    http1_parser_t parser;

    http1_parser_init(&parser, connection, buffer);

    // set default handler

    while (1) {
        int bytes_readed = http1_read_internal(connection, buffer, size);

        switch (bytes_readed) {
        case -1:
            // printf("req 1\n");
            http1_handle(connection);
            // connection->after_read_request(connection);
            return;
        case 0:
            // printf("req 2\n");
            // http1_handle(connection);
            // invoke default handler
            connection->after_read_request(connection);
            // connection->keepalive_enabled = 0;
            // connection->close(connection);
            return;
        default:
            parser.bytes_readed = bytes_readed;

            if (http1_parser_run(&parser) == -1) {
                http1_default_response(connection, 400);
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
    int method_max_length = 7;

    for (parser->pos = parser->pos_start; parser->pos < parser->bytes_readed; parser->pos++) {
        if (parser->buffer[parser->pos] < 32) {
            log_error("HTTP error: denied symbols in method\n");
            return -1;
        }

        if (parser->buffer[parser->pos] == ' ') goto next;

        if (parser->pos - parser->pos_start > method_max_length) {
            log_error("HTTP error: method limit\n");
            return -1;
        }
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
    // int uri_max_length = 8192;

    for (parser->pos = parser->pos_start; parser->pos < parser->bytes_readed; parser->pos++) {
        if (parser->buffer[parser->pos] < 32) {
            log_error("HTTP error: denied symbols in method\n");
            return -1;
        }

        if (parser->buffer[parser->pos] == ' ') goto next;

        // if (parser->pos - parser->pos_start > uri_max_length) {
        //     log_error("HTTP error: uri limit\n");
        //     return -1;
        // }
    }

    return http1_parser_string_append(parser);

    next:

    if (parser->string && http1_parser_string_append(parser) == -1) return -1;

    http1request_t* request = (http1request_t*)parser->connection->request;

    if (parser->string) {
        if (http1_set_uri(request, parser->string, parser->string_len) == -1) return -1;
    } else {
        parser->string_len = parser->pos  - parser->pos_start;

        parser->string = (char*)malloc(parser->string_len + 1);

        if (parser->string == NULL) return -1;

        strncpy(parser->string, &parser->buffer[parser->pos_start], parser->string_len);

        parser->string[parser->string_len] = 0;

        if (http1_set_uri(request, parser->string, parser->string_len) == -1) return -1;
    }

    parser->pos_start = parser->pos + 1;

    parser->string = NULL;

    parser->string_len = 0;

    parser->stage = PROTOCOL;

    if (parser->pos + 1 == parser->bytes_readed) return -2;

    return 0;
}

int http1_parser_parse_protocol(http1_parser_t* parser) {
    int protocol_max_length = 8;

    for (parser->pos = parser->pos_start; parser->pos < parser->bytes_readed; parser->pos++) {
        char ch = parser->buffer[parser->pos];

        if (ch < 32 && !(ch == '\r' || ch == '\n')) {
            log_error("HTTP error: denied symbols in protocol\n");
            return -1;
        }

        if (ch == '\n') {
            if (parser->buffer[parser->pos - 1] == '\r') {
                parser->carriage_return = 1;
            }
            else if (parser->string && parser->string[parser->string_len - 1] == '\r') {
                parser->carriage_return = 1;
            }
            goto next;
        }

        if (parser->pos - parser->pos_start > protocol_max_length) {
            log_error("HTTP error: protocol limit\n");
            return -1;
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

        char ch = parser->buffer[parser->pos];

        if (ch < 32 && !(ch == '\r' || ch == '\n')) return -1;

        if (ch == ' ') {
            // printf("asd %.*s\n", parser->pos - parser->pos_start, &parser->buffer[parser->pos_start]);
            if (parser->buffer[parser->pos - 1] == ':') {
                parser->carriage_return = 1;
            }
            else if (parser->string && parser->string[parser->string_len - 1] == ':') {
                parser->carriage_return = 1;
            }
            goto next;
        }
        else if (ch == '\n') {
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
        parser->string_len = (parser->pos - parser->carriage_return) - parser->pos_start;

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
        char ch = parser->buffer[parser->pos];

        if (ch < 32 && !(ch == '\r' || ch == '\n')) return -1;

        if (ch == '\n') {
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
        parser->string_len = (parser->pos - parser->carriage_return) - parser->pos_start;

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
            request->method = ROUTE_GET;
        }
        else if (string[0] == 'P' && string[1] == 'U' && string[2] == 'T') {
            request->method = ROUTE_PUT;
        }
        break;
    case 4:
        if (string[0] == 'P' && string[1] == 'O' && string[2] == 'S' && string[3] == 'T') {
            request->method = ROUTE_POST;
        }
        break;
    case 5:
        if (string[0] == 'P' && string[1] == 'A' && string[2] == 'T' && string[3] == 'C' && string[4] == 'H') {
            request->method = ROUTE_PATCH;
        }
        break;
    case 6:
        if (string[0] == 'D' && string[1] == 'E' && string[2] == 'L' && string[3] == 'E' && string[4] == 'T' && string[5] == 'E') {
            request->method = ROUTE_DELETE;
        }
        break;
    case 7:
        if (string[0] == 'O' && string[1] == 'P' && string[2] == 'T' && string[3] == 'I' && string[4] == 'O' && string[5] == 'N' && string[6] == 'S') {
            request->method = ROUTE_OPTIONS;
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
    if (string[0] != '/') return -1;

    request->uri = string;
    request->uri_length = length;

    size_t path_point_end = 0;
    size_t ext_point_start = 0;

    for (size_t pos = 0; pos < length; pos++) {
        switch (string[pos]) {
        case '?':
            if (path_point_end == 0) path_point_end = pos;

            int result = http1_set_query(request, string, length, pos);

            if (result == 0) goto next;
            if (result < 0) return result;

            break;
        case '#':
            if (path_point_end == 0) path_point_end = pos;
            goto next;
            break;
        case '.':
            ext_point_start = pos + 1;
            break;
        }
    }

    next:

    if (path_point_end == 0) path_point_end = length;

    if (http1_set_path(request, string, path_point_end) == -1) return -1;

    if (ext_point_start == 0) ext_point_start = path_point_end;

    if (http1_set_ext(request, &string[ext_point_start], path_point_end - ext_point_start) == -1) return -1;

    return 0;
}

int http1_set_path(http1request_t* request, const char* string, size_t length) {
    char* path = (char*)malloc(length + 1);

    if (path == NULL) return -1;

    strncpy(path, string, length);

    path[length] = 0;

    request->path = path;
    request->path_length = length;

    return 0;
}

int http1_set_ext(http1request_t* request, const char* string, size_t length) {
    char* ext = (char*)malloc(length + 1);

    if (ext == NULL) return -1;

    strncpy(ext, string, length);

    ext[length] = 0;

    request->ext = ext;
    request->ext_length = length;

    return 0;
}

int http1_set_query(http1request_t* request, const char* string, size_t length, size_t pos) {
    size_t point_start = pos + 1;

    enum { KEY, VALUE } stage = KEY;

    http1_query_t* query = http1_query_create();

    if (query == NULL) return -1;

    request->query = query;
    request->last_query = query;

    for (++pos; pos < length; pos++) {
        switch (string[pos]) {
        case '=':
            if (string[pos - 1] == '=') continue;

            stage = VALUE;

            query->key = http1_query_set_field(&string[point_start], pos - point_start);

            if (query->key == NULL) return -1;

            point_start = pos + 1;
            break;
        case '&':
            stage = KEY;

            query->value = http1_query_set_field(&string[point_start], pos - point_start);

            if (query->value == NULL) return -1;

            http1_query_t* query_new = http1_query_create();

            if (query_new == NULL) return -1;

            http1_append_query(request, query_new);

            query = query_new;

            point_start = pos + 1;
            break;
        case '#':
            if (stage == KEY) {
                query->key = http1_query_set_field(&string[point_start], pos - point_start);
                if (query->key == NULL) return -1;
            }
            else if (stage == VALUE) {
                query->value = http1_query_set_field(&string[point_start], pos - point_start);
                if (query->value == NULL) return -1;
            }

            return 0;
        }
    }

    if (stage == KEY) {
        query->key = http1_query_set_field(&string[point_start], pos - point_start);
        if (query->key == NULL) return -1;

        query->value = http1_query_set_field("", 0);
        if (query->value == NULL) return -1;
    }
    else if (stage == VALUE) {
        query->value = http1_query_set_field(&string[point_start], pos - point_start);
        if (query->value == NULL) return -1;
    }

    return 0;
}

void http1_append_query(http1request_t* request, http1_query_t* query) {
    if (request->last_query) {
        request->last_query->next = query;
    }

    request->last_query = query;
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
    http1_header_t* header = http1_header_create(key, value);

    if (header == NULL) {
        log_error("HTTP error: can't alloc header memory\n");
        return -1;
    }

    if (request->header == NULL) {
        request->header = header;
    }

    if (request->last_header) {
        request->last_header->next = header;
    }

    request->last_header = header;

    return 0;
}

http1_header_t* http1_header_alloc() {
    return (http1_header_t*)malloc(sizeof(http1_header_t));
}

http1_header_t* http1_header_create(const char* key, const char* value) {
    http1_header_t* header = http1_header_alloc();

    header->key = key;
    header->value = value;
    header->next = NULL;

    return header;
}

http1_query_t* http1_query_alloc() {
    return (http1_query_t*)malloc(sizeof(http1_query_t));
}

http1_query_t* http1_query_create() {
    http1_query_t* query = http1_query_alloc();

    if (query == NULL) {
        log_error("HTTP error: can't alloc query\n");
        return NULL;
    }

    query->key = NULL;
    query->value = NULL;
    query->next = NULL;

    return query;
}

const char* http1_query_set_field(const char* string, size_t length) {
    char* field = (char*)calloc(length + 1, sizeof(char));

    if (field == NULL) {
        log_error("HTTP error: can't alloc query field\n");
        return NULL;
    }

    strncpy(field, string, length);

    field[length] = 0;

    return field;
}

void http1_handle(connection_t* connection) {
    if (http1_get_resource(connection) == -1) {
        // connection->after_read_request(connection);
        return;
    }

    if (http1_get_file(connection) == -1) {
        connection->after_read_request(connection);
        return;
    }

    http1_default_response(connection, 404);

    connection->after_read_request(connection);

    {
        // printf("method %d\n", request->method);
        // printf("version %d\n", request->version);
        // printf("uri %s\n", request->uri);
        // printf("path %s\n", request->path);

        // http1_header_t* header = request->header;

        // while (header) {
        //     printf("%s: %s\n", header->key, header->value);

        //     header = header->next;
        // }

        // http1_query_t* query = request->query;

        // while (query) {
        //     printf("%s: %s\n", query->key, query->value);

        //     query = query->next;
        // }

        // printf("\n\n");
    }

    return;
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

                http1_query_t* query = http1_query_create();

                if (query == NULL) return -1;

                query->key = http1_query_set_field(param->string, param->string_len);
                if (query->key == NULL) return -1;

                query->value = http1_query_set_field(&request->path[vector[i * 2]], substring_length);
                if (query->value == NULL) return -1;

                http1_append_query(request, query);
            }

            connection->handle = route->method[request->method];
            connection->queue_push(connection);

            // route->method[request->method]((char*)request->path);

            return 0;
        }
    }

    return 0;
}

int http1_get_file(connection_t* connection) {
    http1request_t* request = (http1request_t*)connection->request;
    http1response_t* response = (http1response_t*)connection->response;

    const char* mimetype = http1_get_mimetype(request->ext);

    response->headern_add("Content-Type", 12, mimetype, strlen(mimetype));

    size_t fullpath_length = connection->server->root_length + connection->request->path_length;

    char* fullpath = (char*)malloc(fullpath_length + 1);

    if (fullpath == NULL) return -1;

    strncpy(&fullpath[0], connection->server->root, connection->server->root_length);
    strncat(&fullpath[connection->server->root_length], connection->request->path, connection->request->path_length);

    fullpath[fullpath_length] = 0;

    struct stat stat_obj;

    stat(a, &stat_obj);

    if (S_ISDIR(stat_obj.st_mode)) {
        http1_default_response(connection, 403);
        return 0;
    }

    file_fd = open(full_path, O_RDWR);

    ssize_t file_size = 0;
    ssize_t file_size_part = 0;

    if (file_fd == -1) {
        http1_default_response(connection, 404);
        return 1;
    }

    file_size = (unsigned int)lseek(file_fd, 0, SEEK_END);

    lseek(file_fd, 0, SEEK_SET);

    return 0;
}

const char* http1_get_mimetype(const char* extension) {
    const char* mimetype = mimetype_find_type(extension);

    if (mimetype == NULL) return "text/plain";

    return mimetype;
}

void http1_default_response(connection_t* connection, int status_code) {

}

void http1_get_redirect(connection_t* connection) {
    http1request_t* request = (http1request_t*)connection->request;

    redirect_t* redirect = connection->server->redirect;

    for (; redirect; redirect = redirect->next) {
        
    }
}