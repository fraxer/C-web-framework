#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include "../connection/connection.h"
#include "../request/http1request.h"
#include "../response/http1response.h"
#include "../domain/domain.h"
#include "../log/log.h"
#include "http1common.h"
#include "http1parser.h"

int http1parser_parse_payload(http1parser_t*);
int http1parser_set_method(http1request_t*, http1parser_t*);
int http1parser_set_protocol(http1request_t*, http1parser_t*);
int http1parser_set_header_key(http1request_t*, http1parser_t*);
int http1parser_set_header_value(http1request_t*, http1parser_t*);
int http1parser_set_path(http1request_t*, const char*, size_t);
int http1parser_set_ext(http1request_t*, const char*, size_t);
int http1parser_set_query(http1request_t*, const char*, size_t, size_t);
int http1parser_host_not_found(http1parser_t*);
void http1parser_try_set_keepalive(http1parser_t*);
void http1parser_try_set_range(http1parser_t*);
int http1parser_cmplower(const char*, ssize_t, const char*, ssize_t);
int http1parser_is_ctl(int);


void http1parser_init(http1parser_t* parser, connection_t* connection, char* buffer) {
    parser->stage = METHOD;
    parser->host_found = 0;
    parser->bytes_readed = 0;
    parser->pos_start = 0;
    parser->pos = 0;
    parser->buffer = buffer;
    parser->connection = connection;

    parser->buf.dynamic_buffer = NULL;
    parser->buf.offset_sbuffer = 0;
    parser->buf.offset_dbuffer = 0;
    parser->buf.dbuffer_size = 0;
    parser->buf.type = STATIC;
}

void http1parser_free(http1parser_t* parser) {
    if (parser->buf.dynamic_buffer) free(parser->buf.dynamic_buffer);
    parser->buf.dynamic_buffer = NULL;
}

int http1parser_run(http1parser_t* parser) {
    http1request_t* request = (http1request_t*)parser->connection->request;
    parser->pos_start = 0;
    parser->pos = 0;

    if (parser->stage == PAYLOAD)
        return http1parser_parse_payload(parser);

    for (parser->pos = parser->pos_start; parser->pos < parser->bytes_readed; parser->pos++) {
        char ch = parser->buffer[parser->pos];

        switch (parser->stage)
        {
        case METHOD:
            if (ch == ' ') {
                parser->stage = URI;

                http1parser_buffer_complete(parser);
                if (!http1parser_set_method(request, parser))
                    return HTTP1PARSER_BAD_REQUEST;

                http1parser_buffer_reset(parser);
                break;
            }
            else {
                if (http1parser_buffer_writed(parser) > 7)
                    return HTTP1PARSER_BAD_REQUEST;

                http1parser_buffer_push(parser, ch);
                break;
            }
        case URI:
            if (ch == ' ') {
                parser->stage = PROTOCOL;

                http1parser_buffer_complete(parser);

                size_t length = http1parser_buffer_writed(parser);
                char* uri = http1parser_buffer_copy(parser);
                if (uri == NULL) 
                    return HTTP1PARSER_OUT_OF_MEMORY;

                int result = http1parser_set_uri(request, uri, length);
                if (result != HTTP1PARSER_SUCCESS)
                    return result;

                http1parser_buffer_reset(parser);
                break;
            }
            else if (http1parser_is_ctl(ch)) {
                return HTTP1PARSER_BAD_REQUEST;
            }
            else {
                http1parser_buffer_push(parser, ch);
                break;
            }
        case PROTOCOL:
            if (ch == '\r') {
                parser->stage = NEWLINE1;

                http1parser_buffer_complete(parser);
                if (!http1parser_set_protocol(request, parser))
                    return HTTP1PARSER_BAD_REQUEST;

                http1parser_buffer_reset(parser);

                break;
            }
            else {
                if (http1parser_buffer_writed(parser) > 8)
                    return HTTP1PARSER_BAD_REQUEST;

                http1parser_buffer_push(parser, ch);
                break;
            }
        case NEWLINE1:
            if (ch == '\n') {
                parser->stage = HEADER_KEY;
                break;
            }
            else
                return HTTP1PARSER_BAD_REQUEST;
        case HEADER_KEY:
            if (ch == '\r') {
                if (http1parser_buffer_writed(parser) > 0)
                    return HTTP1PARSER_BAD_REQUEST;

                parser->stage = NEWLINE3;
                break;
            }
            else if (ch == ':') {
                parser->stage = HEADER_SPACE;

                http1parser_buffer_complete(parser);
                if (!http1parser_set_header_key(request, parser))
                    return HTTP1PARSER_BAD_REQUEST;

                http1parser_buffer_reset(parser);

                break;
            }
            else if (http1parser_is_ctl(ch)) {
                return HTTP1PARSER_BAD_REQUEST;
            }
            else {
                http1parser_buffer_push(parser, ch);
                break;
            }
        case HEADER_SPACE:
            if (ch == ' ') {
                parser->stage = HEADER_VALUE;
                break;
            }
            else
                return HTTP1PARSER_BAD_REQUEST;
        case HEADER_VALUE:
            if (ch == '\r') {
                parser->stage = NEWLINE2;

                http1parser_buffer_complete(parser);
                if (!http1parser_set_header_value(request, parser))
                    return HTTP1PARSER_BAD_REQUEST;

                http1parser_buffer_reset(parser);

                break;
            }
            else if (http1parser_is_ctl(ch)) {
                return HTTP1PARSER_BAD_REQUEST;
            }
            else
            {
                http1parser_buffer_push(parser, ch);
                break;
            }
        case NEWLINE2:
            if (ch == '\n') {
                parser->stage = HEADER_KEY;
                break;
            }
            else
                return HTTP1PARSER_BAD_REQUEST;
        case NEWLINE3:
            if (ch == '\n') {
                parser->stage = PAYLOAD;
                break;
            }
            else
                return HTTP1PARSER_BAD_REQUEST;
        case PAYLOAD:
            return http1parser_parse_payload(parser);
        }
    }

    return HTTP1PARSER_SUCCESS;
}

void http1parser_set_bytes_readed(struct http1parser* parser, int readed) {
    parser->bytes_readed = readed;
}

int http1parser_parse_payload(http1parser_t* parser) {
    http1request_t* request = (http1request_t*)parser->connection->request;

    switch (request->method) {
    case ROUTE_POST:
    case ROUTE_PUT:
    case ROUTE_PATCH:
        break;
    default:
        return HTTP1PARSER_BAD_REQUEST;
    }

    parser->pos_start = parser->pos;
    parser->pos = parser->bytes_readed;

    if (request->payload_.fd <= 0) {
        const char* template = "tmp.XXXXXX";
        const char* tmp_dir = parser->connection->server->info->tmp_dir;

        size_t path_length = strlen(tmp_dir) + strlen(template) + 2; // "/", "\0"
        request->payload_.path = malloc(path_length);
        snprintf(request->payload_.path, path_length, "%s/%s", tmp_dir, template);

        request->payload_.fd = mkstemp(request->payload_.path);
        if (request->payload_.fd == -1) return HTTP1PARSER_ERROR;
    }

    size_t string_len = parser->pos - parser->pos_start;
    off_t payload_length = lseek(request->payload_.fd, 0, SEEK_END);

    if (payload_length + string_len > parser->connection->server->info->client_max_body_size) {
        return HTTP1PARSER_PAYLOAD_LARGE;
    }

    int r = write(request->payload_.fd, &parser->buffer[parser->pos_start], string_len);
    lseek(request->payload_.fd, 0, SEEK_SET);
    if (r <= 0) return HTTP1PARSER_ERROR;

    return HTTP1PARSER_CONTINUE;
}

int http1parser_set_method(http1request_t* request, http1parser_t* parser) {
    char* string = http1parser_buffer_get(parser);
    size_t length = http1parser_buffer_writed(parser);

    if (length == 3 && string[0] == 'G' && string[1] == 'E' && string[2] == 'T') {
        request->method = ROUTE_GET;
    }
    else if (length == 3 && string[0] == 'P' && string[1] == 'U' && string[2] == 'T') {
        request->method = ROUTE_PUT;
    }
    else if (length == 4 && string[0] == 'P' && string[1] == 'O' && string[2] == 'S' && string[3] == 'T') {
        request->method = ROUTE_POST;
    }
    else if (length == 5 && string[0] == 'P' && string[1] == 'A' && string[2] == 'T' && string[3] == 'C' && string[4] == 'H') {
        request->method = ROUTE_PATCH;
    }
    else if (length == 6 && string[0] == 'D' && string[1] == 'E' && string[2] == 'L' && string[3] == 'E' && string[4] == 'T' && string[5] == 'E') {
        request->method = ROUTE_DELETE;
    }
    else if (length == 7 && string[0] == 'O' && string[1] == 'P' && string[2] == 'T' && string[3] == 'I' && string[4] == 'O' && string[5] == 'N' && string[6] == 'S') {
        request->method = ROUTE_OPTIONS;
    }
    else return 0;

    return 1;
}

int http1parser_set_uri(http1request_t* request, const char* string, size_t length) {
    if (string[0] != '/') return HTTP1PARSER_BAD_REQUEST;

    http1_urlendec_t st = http1_urldecode(string, length);

    free((void*)string);

    string = st.string;
    request->uri = st.string;
    request->uri_length = st.length;

    size_t path_point_end = 0;
    size_t ext_point_start = 0;

    for (size_t pos = 0; pos < length; pos++) {
        switch (string[pos]) {
        case '?':
            if (path_point_end == 0) path_point_end = pos;

            int result = http1parser_set_query(request, string, length, pos);

            if (result == HTTP1PARSER_SUCCESS) goto next;
            else return result;

            break;
        case '#':
            if (path_point_end == 0) path_point_end = pos;
            goto next;
            break;
        }
    }

    next:

    if (path_point_end == 0) path_point_end = length;

    int result = http1parser_set_path(request, string, path_point_end);

    if (result != HTTP1PARSER_SUCCESS) return result;

    if (ext_point_start == 0) ext_point_start = path_point_end;

    result = http1parser_set_ext(request, &string[ext_point_start], path_point_end - ext_point_start);

    if (result != HTTP1PARSER_SUCCESS) return result;

    return HTTP1PARSER_SUCCESS;
}

int http1parser_set_protocol(http1request_t* request, http1parser_t* parser) {
    char* string = http1parser_buffer_get(parser);

    if (string[0] == 'H' && string[1] == 'T' && string[2] == 'T' && string[3] == 'P' && string[4] == '/'  && string[5] == '1' && string[6] == '.' && string[7] == '1') {
        request->version = HTTP1_VER_1_1;
        return HTTP1PARSER_SUCCESS;
    }
    else if (string[0] == 'H' && string[1] == 'T' && string[2] == 'T' && string[3] == 'P' && string[4] == '/'  && string[5] == '1' && string[6] == '.' && string[7] == '0') {
        request->version = HTTP1_VER_1_0;
        return HTTP1PARSER_SUCCESS;
    }

    log_error("HTTP error: version error\n");

    return HTTP1PARSER_BAD_REQUEST;
}

int http1parser_set_path(http1request_t* request, const char* string, size_t length) {
    char* path = (char*)malloc(length + 1);

    if (path == NULL) return HTTP1PARSER_OUT_OF_MEMORY;

    memcpy(path, string, length);

    path[length] = 0;

    request->path = path;
    request->path_length = length;

    char ch_1 = 0;
    char ch_2 = 0;
    char ch_3 = 0;
    for (size_t i = 0; i <= length; i++) {
        if (ch_1 == '/' && ch_2 == '.' && ch_3 == '.' && path[i] == '/') {
            return HTTP1PARSER_BAD_REQUEST;
        }
        else if (ch_1 == '/' && ch_2 == '.' && ch_3 == '.' && path[i] == '\0') {
            return HTTP1PARSER_BAD_REQUEST;
        }

        ch_1 = ch_2;
        ch_2 = ch_3;
        ch_3 = path[i];
    }

    return HTTP1PARSER_SUCCESS;
}

int http1parser_set_ext(http1request_t* request, const char* string, size_t length) {
    char* ext = (char*)malloc(length + 1);

    if (ext == NULL) return HTTP1PARSER_OUT_OF_MEMORY;

    strncpy(ext, string, length);

    ext[length] = 0;

    request->ext = ext;
    request->ext_length = length;

    return HTTP1PARSER_SUCCESS;
}

int http1parser_set_query(http1request_t* request, const char* string, size_t length, size_t pos) {
    size_t point_start = pos + 1;

    enum { KEY, VALUE } stage = KEY;

    http1_query_t* query = http1_query_create(NULL, 0, NULL, 0);

    if (query == NULL) return HTTP1PARSER_OUT_OF_MEMORY;

    request->query = query;
    request->last_query = query;

    for (++pos; pos < length; pos++) {
        switch (string[pos]) {
        case '=':
            if (string[pos - 1] == '=') continue;

            stage = VALUE;

            query->key = http1_set_field(&string[point_start], pos - point_start);

            if (query->key == NULL) return HTTP1PARSER_OUT_OF_MEMORY;

            point_start = pos + 1;
            break;
        case '&':
            stage = KEY;

            query->value = http1_set_field(&string[point_start], pos - point_start);

            if (query->value == NULL) return HTTP1PARSER_OUT_OF_MEMORY;

            http1_query_t* query_new = http1_query_create(NULL, 0, NULL, 0);

            if (query_new == NULL) return HTTP1PARSER_OUT_OF_MEMORY;

            http1parser_append_query(request, query_new);

            query = query_new;

            point_start = pos + 1;
            break;
        case '#':
            if (stage == KEY) {
                query->key = http1_set_field(&string[point_start], pos - point_start);
                if (query->key == NULL) return HTTP1PARSER_OUT_OF_MEMORY;
            }
            else if (stage == VALUE) {
                query->value = http1_set_field(&string[point_start], pos - point_start);
                if (query->value == NULL) return HTTP1PARSER_OUT_OF_MEMORY;
            }

            return HTTP1PARSER_SUCCESS;
        }
    }

    if (stage == KEY) {
        query->key = http1_set_field(&string[point_start], pos - point_start);
        if (query->key == NULL) return HTTP1PARSER_OUT_OF_MEMORY;

        query->value = http1_set_field("", 0);
        if (query->value == NULL) return HTTP1PARSER_OUT_OF_MEMORY;
    }
    else if (stage == VALUE) {
        query->value = http1_set_field(&string[point_start], pos - point_start);
        if (query->value == NULL) return HTTP1PARSER_OUT_OF_MEMORY;
    }

    return HTTP1PARSER_SUCCESS;
}

void http1parser_append_query(http1request_t* request, http1_query_t* query) {
    if (request->last_query) {
        request->last_query->next = query;
    }

    request->last_query = query;
}

int http1parser_host_not_found(http1parser_t* parser) {
    if (parser->host_found) return HTTP1PARSER_SUCCESS;

    http1request_t* request = (http1request_t*)parser->connection->request;

    http1_header_t* header = request->last_header;

    const char* host_key = "host";
    size_t host_key_length = 4;

    for (size_t i = 0, j = 0; i < header->key_length && j < host_key_length; i++, j++) {
        if (tolower(header->key[i]) != tolower(host_key[j])) return HTTP1PARSER_CONTINUE;
    }

    domain_t* domain = parser->connection->server->domain;

    int vector_struct_size = 6;
    int substring_count = 20;
    int vector_size = substring_count * vector_struct_size;
    int vector[vector_size];

    while (domain) {
        int matches_count = pcre_exec(domain->pcre_template, NULL, header->value, header->value_length, 0, 0, vector, vector_size);

        if (matches_count > 0) {
            parser->host_found = 1;
            return HTTP1PARSER_SUCCESS;
        }

        domain = domain->next;
    }

    return HTTP1PARSER_BAD_REQUEST;
}

void http1parser_try_set_keepalive(http1parser_t* parser) {
    http1request_t* request = (http1request_t*)parser->connection->request;
    http1_header_t* header = request->last_header;

    const char* connection_key = "connection";
    ssize_t connection_key_length = 10;
    const char* connection_value = "keep-alive";
    ssize_t connection_value_length = 10;

    if ((ssize_t)header->key_length != connection_key_length) return;
    if (!http1parser_cmplower(header->key, header->key_length, connection_key, connection_key_length)) return;

    parser->connection->keepalive_enabled = 0;
    if (http1parser_cmplower(header->value, header->value_length, connection_value, connection_value_length)) {
        parser->connection->keepalive_enabled = 1;
    }
}

void http1parser_try_set_range(http1parser_t* parser) {
    http1request_t* request = (http1request_t*)parser->connection->request;
    http1response_t* response = (http1response_t*)parser->connection->response;

    if (http1parser_cmplower(request->last_header->key, request->last_header->key_length, "range", 5)) {
        response->ranges = http1parser_parse_range((char*)request->last_header->value, request->last_header->value_length);
    }
}

int http1parser_cmplower(const char* a, ssize_t a_length, const char* b, ssize_t b_length) {
    for (ssize_t i = 0, j = 0; i < a_length && j < b_length; i++, j++) {
        if (tolower(a[i]) != tolower(b[j])) return 0;
    }

    return 1;
}

http1_ranges_t* http1parser_parse_range(char* str, size_t length) {
    int result = -1;
    int max = 2147483647;
    long long int start_finded = 0;
    size_t start_position = 6;
    http1_ranges_t* ranges = NULL;
    http1_ranges_t* last_range = NULL;

    if (!(str[0] == 'b' && str[1] == 'y' && str[2] == 't' && str[3] == 'e' && str[4] == 's' && str[5] == '=')) return NULL;

    for (size_t i = start_position; i <= length; i++) {
        long long int end = -1;

        if (isdigit(str[i])) continue;
        else if (str[i] == '-') {
            if (last_range && last_range->end == -1) goto failed;
            if (last_range && last_range->start == -1) goto failed;

            long long int start = -1;

            start_finded = 1;
            if (i > start_position) {
                if (i - start_position > 10) goto failed;

                str[i] = 0;
                start = atoll(&str[start_position]);
                str[i] = '-';

                if (start > max) goto failed;

                if (last_range && last_range->start > start) goto failed;

                if (last_range && last_range->start > -1 && last_range->end >= start) {
                    start_position = i + 1;
                    continue;
                }
            }

            http1_ranges_t* range = http1response_init_ranges();
            if (range == NULL) goto failed;

            if (ranges == NULL) ranges = range;

            if (last_range != NULL) {
                last_range->next = range;
            }
            last_range = range;

            if (i > start_position) {
                range->start = start;
            }

            start_position = i + 1;
        }
        else if (str[i] == ',') {
            if (i > start_position) {
                if (i - start_position > 10) goto failed;

                str[i] = 0;
                end = atoll(&str[start_position]);
                str[i] = ',';

                if (end > max) goto failed;
                if (end < last_range->start) goto failed;
                if (start_finded == 0) goto failed;

                if (last_range->end <= end) {
                    last_range->end = end;
                }
            }

            start_finded = 0;
            start_position = i + 1;
        }
        else if (str[i] == 0) {
            if (i > start_position) {
                if (i - start_position > 10) goto failed;

                end = atoll(&str[start_position]);

                if (end > max) goto failed;
                if (end < last_range->start) goto failed;
                if (start_finded == 0) goto failed;

                if (last_range->end <= end) {
                    last_range->end = end;
                }
            }
            else if (last_range && start_finded) {
                last_range->end = -1;
            }
        }
        else if (str[i] == ' ') {
            if (!(i > 0 && str[i - 1] == ',')) goto failed;
            start_position = i + 1;
        }
        else goto failed;
    }

    result = 0;

    failed:

    if (result == -1) {
        http1response_free_ranges(ranges);
        return NULL;
    }

    return ranges;
}

int http1parser_buffer_push(http1parser_t* parser, char ch) {
    parser->buf.static_buffer[parser->buf.offset_sbuffer] = ch;
    parser->buf.offset_sbuffer++;

    if (parser->buf.offset_sbuffer < HTTP1PARSER_BUFSIZ)
        return HTTP1PARSER_SUCCESS;

    parser->buf.type = DYNAMIC;

    return http1parser_buffer_move(parser);
}

void http1parser_buffer_reset(http1parser_t* parser) {
    parser->buf.offset_dbuffer = 0;
    parser->buf.offset_sbuffer = 0;
    parser->buf.type = STATIC;
}

size_t http1parser_buffer_writed(http1parser_t* parser) {
    if (parser->buf.type == DYNAMIC) {
        return parser->buf.offset_dbuffer + parser->buf.offset_sbuffer;
    }
    return parser->buf.offset_sbuffer;
}

int http1parser_buffer_complete(http1parser_t* parser) {
    if (parser->buf.type == DYNAMIC) {
        return http1parser_buffer_move(parser);
    }
    return HTTP1PARSER_SUCCESS;
}

int http1parser_buffer_move(http1parser_t* parser) {
    if (parser->buf.type != DYNAMIC)
        return HTTP1PARSER_SUCCESS;

    size_t dbuffer_length = parser->buf.offset_dbuffer + parser->buf.offset_sbuffer;

    if (parser->buf.dbuffer_size <= dbuffer_length) {
        char* data = realloc(parser->buf.dynamic_buffer, dbuffer_length + 1);

        if (data == NULL)
            return HTTP1PARSER_OUT_OF_MEMORY;

        parser->buf.dbuffer_size = dbuffer_length + 1;
        parser->buf.dynamic_buffer = data;
    }

    memcpy(&parser->buf.dynamic_buffer[parser->buf.offset_dbuffer], parser->buf.static_buffer, parser->buf.offset_sbuffer);

    parser->buf.dynamic_buffer[dbuffer_length] = 0;
    parser->buf.offset_dbuffer = dbuffer_length;
    parser->buf.offset_sbuffer = 0;

    return HTTP1PARSER_SUCCESS;
}

char* http1parser_buffer_get(http1parser_t* parser) {
    if (parser->buf.type == DYNAMIC) {
        return parser->buf.dynamic_buffer;
    }
    return parser->buf.static_buffer;
}

char* http1parser_buffer_copy(http1parser_t* parser) {
    char* string = http1parser_buffer_get(parser);
    size_t length = http1parser_buffer_writed(parser);

    char* data = malloc(length + 1);
    if (data == NULL) return NULL;

    memcpy(data, string, length);
    data[length] = 0;

    return data;
}

int http1parser_set_header_key(http1request_t* request, http1parser_t* parser) {
    char* string = http1parser_buffer_get(parser);
    size_t length = http1parser_buffer_writed(parser);
    http1_header_t* header = http1_header_create(NULL, 0, NULL, 0);

    if (header == NULL) {
        log_error("HTTP error: can't alloc header memory\n");
        return HTTP1PARSER_OUT_OF_MEMORY;
    }
    
    header->key = http1_set_field(string, length);
    header->key_length = length;

    if (header->key == NULL)
        return HTTP1PARSER_OUT_OF_MEMORY;

    if (request->header_ == NULL) {
        request->header_ = header;
    }

    if (request->last_header) {
        request->last_header->next = header;
    }

    request->last_header = header;

    return HTTP1PARSER_SUCCESS;
}

int http1parser_set_header_value(http1request_t* request, http1parser_t* parser) {
    char* string = http1parser_buffer_get(parser);
    size_t length = http1parser_buffer_writed(parser);

    request->last_header->value = http1_set_field(string, length);
    request->last_header->value_length = length;

    if (request->last_header->value == NULL)
        return HTTP1PARSER_OUT_OF_MEMORY;

    if (http1parser_host_not_found(parser) == HTTP1PARSER_BAD_REQUEST)
        return HTTP1PARSER_BAD_REQUEST;

    http1parser_try_set_keepalive(parser);
    http1parser_try_set_range(parser);

    return HTTP1PARSER_SUCCESS;
}

int http1parser_is_ctl(int c) {
    return (c >= 0 && c <= 31) || (c == 127);
}