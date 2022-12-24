#include <string.h>
#include "../connection/connection.h"
#include "../request/http1request.h"
#include "../log/log.h"
#include "http1parser.h"
    #include <stdio.h>

int http1_parser_parse_method(http1_parser_t*);
int http1_parser_parse_uri(http1_parser_t*);
int http1_parser_parse_protocol(http1_parser_t*);
int http1_parser_parse_headers_key(http1_parser_t*);
int http1_parser_parse_headers_value(http1_parser_t*);
int http1_parser_parse_header_key(http1_parser_t*);
int http1_parser_parse_header_value(http1_parser_t*);
int http1_parser_parse_endhead(http1_parser_t*);
int http1_parser_parse_payload(http1_parser_t*);
int http1_parser_string_append(http1_parser_t*);
int http1_parser_set_method(http1request_t*, const char*, size_t);
int http1_parser_set_uri(http1request_t*, const char*, size_t);
int http1_parser_set_protocol(http1request_t*, const char*);
int http1_parser_set_header(http1request_t*, const char*, size_t, const char*, size_t);
int http1_parser_set_path(http1request_t*, const char*, size_t);
int http1_parser_set_ext(http1request_t*, const char*, size_t);
int http1_parser_set_query(http1request_t*, const char*, size_t, size_t);


void http1_parser_init(http1_parser_t* parser, connection_t* connection, char* buffer) {
    parser->stage = METHOD;
    parser->carriage_return = 0;
    parser->bytes_readed = 0;
    parser->pos_start = 0;
    parser->pos = 0;
    parser->string_len = 0;
    parser->string = NULL;
    parser->buffer = buffer;
    parser->connection = connection;
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

int http1_parser_set_bytes_readed(struct http1_parser* parser, int readed) {
    parser->bytes_readed = readed;
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
        if (http1_parser_set_method(request, parser->string, parser->string_len) == -1) return -1;
    } else {
        if (http1_parser_set_method(request, &parser->buffer[parser->pos_start], parser->pos - parser->pos_start) == -1) return -1;
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
        if (http1_parser_set_uri(request, parser->string, parser->string_len) == -1) return -1;
    } else {
        parser->string_len = parser->pos  - parser->pos_start;

        parser->string = (char*)malloc(parser->string_len + 1);

        if (parser->string == NULL) return -1;

        strncpy(parser->string, &parser->buffer[parser->pos_start], parser->string_len);

        parser->string[parser->string_len] = 0;

        if (http1_parser_set_uri(request, parser->string, parser->string_len) == -1) return -1;
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
        if (http1_parser_set_protocol(request, parser->string) == -1) return -1;
    } else {
        if (http1_parser_set_protocol(request, &parser->buffer[parser->pos_start]) == -1) return -1;
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
        if (http1_parser_set_header(request, parser->string, parser->string_len, header_value, 0) == -1) return -1;
    } else {
        parser->string_len = (parser->pos - parser->carriage_return) - parser->pos_start;

        parser->string = (char*)malloc(parser->string_len + 1);

        if (parser->string == NULL) return -1;

        strncpy(parser->string, &parser->buffer[parser->pos_start], parser->string_len);

        parser->string[parser->string_len] = 0;

        if (http1_parser_set_header(request, parser->string, parser->string_len, header_value, 0) == -1) return -1;
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
        request->last_header->value_length = parser->string_len;
    } else {
        parser->string_len = (parser->pos - parser->carriage_return) - parser->pos_start;

        parser->string = (char*)malloc(parser->string_len + 1);

        if (parser->string == NULL) return -1;

        strncpy(parser->string, &parser->buffer[parser->pos_start], parser->string_len);

        parser->string[parser->string_len] = 0;

        request->last_header->value = parser->string;
        request->last_header->value_length = parser->string_len;
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

int http1_parser_set_method(http1request_t* request, const char* string, size_t length) {
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

int http1_parser_set_uri(http1request_t* request, const char* string, size_t length) {
    if (string[0] != '/') return -1;

    request->uri = string;
    request->uri_length = length;

    size_t path_point_end = 0;
    size_t ext_point_start = 0;

    for (size_t pos = 0; pos < length; pos++) {
        switch (string[pos]) {
        case '?':
            if (path_point_end == 0) path_point_end = pos;

            int result = http1_parser_set_query(request, string, length, pos);

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

    if (http1_parser_set_path(request, string, path_point_end) == -1) return -1;

    if (ext_point_start == 0) ext_point_start = path_point_end;

    if (http1_parser_set_ext(request, &string[ext_point_start], path_point_end - ext_point_start) == -1) return -1;

    return 0;
}

int http1_parser_set_protocol(http1request_t* request, const char* string) {
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

int http1_parser_set_header(http1request_t* request, const char* key, size_t key_length, const char* value, size_t value_length) {
    http1_header_t* header = http1_header_create(key, key_length, value, value_length);

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

int http1_parser_set_path(http1request_t* request, const char* string, size_t length) {
    char* path = (char*)malloc(length + 1);

    if (path == NULL) return -1;

    strncpy(path, string, length);

    path[length] = 0;

    request->path = path;
    request->path_length = length;

    return 0;
}

int http1_parser_set_ext(http1request_t* request, const char* string, size_t length) {
    char* ext = (char*)malloc(length + 1);

    if (ext == NULL) return -1;

    strncpy(ext, string, length);

    ext[length] = 0;

    request->ext = ext;
    request->ext_length = length;

    return 0;
}

int http1_parser_set_query(http1request_t* request, const char* string, size_t length, size_t pos) {
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

            http1_parser_append_query(request, query_new);

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

void http1_parser_append_query(http1request_t* request, http1_query_t* query) {
    if (request->last_query) {
        request->last_query->next = query;
    }

    request->last_query = query;
}