#include <string.h>

#include "config.h"
#include "websocketsprotocolresource.h"
#include "websocketsparser.h"

static const size_t method_max_length = 6;

int websocketsrequest_get_resource(connection_t*);
void websockets_protocol_resource_reset(void*);
void websockets_protocol_resource_free(void*);
int websockets_protocol_resource_payload_parse(websocketsrequest_t*, char*, size_t);
const char* websocketsrequest_query(websockets_protocol_resource_t*, const char*);
websockets_query_t* websocketsrequest_last_query_item(websockets_protocol_resource_t*);
void websocketsrequest_query_free(websockets_query_t*);
int websocketsrequest_has_payload(websockets_protocol_resource_t*);
int websocketsparser_parse_method(websockets_protocol_resource_t*, char*, size_t);
int websocketsparser_parse_location(websockets_protocol_resource_t*, char*, size_t);
int websocketsparser_append_uri(websockets_protocol_resource_t*, const char*, size_t);
int websocketsparser_set_path(websockets_protocol_resource_t*, const char*, size_t);
int websocketsparser_set_ext(websockets_protocol_resource_t*, const char*, size_t);
int websocketsparser_set_query(websockets_protocol_resource_t*, const char*, size_t);

websockets_protocol_t* websockets_protocol_resource_create() {
    websockets_protocol_resource_t* protocol = malloc(sizeof * protocol);
    if (protocol == NULL) return NULL;

    protocol->base.payload_parse = websockets_protocol_resource_payload_parse;
    protocol->base.get_resource = websocketsrequest_get_resource;
    protocol->base.reset = websockets_protocol_resource_reset;
    protocol->base.free = websockets_protocol_resource_free;
    protocol->payload = (char*(*)(websockets_protocol_resource_t*))websocketsrequest_payload;
    protocol->payload_json = (jsondoc_t*(*)(websockets_protocol_resource_t*))websocketsrequest_payload_json;
    protocol->method = ROUTE_NONE;
    protocol->parser_stage = WSPROTRESOURCE_METHOD;
    protocol->uri_length = 0;
    protocol->path_length = 0;
    protocol->ext_length = 0;
    protocol->uri = NULL;
    protocol->path = NULL;
    protocol->ext = NULL;
    protocol->query_ = NULL;
    protocol->query = websocketsrequest_query;

    websockets_protocol_init_payload((websockets_protocol_t*)protocol);

    return (websockets_protocol_t*)protocol;
}

void websockets_protocol_resource_reset(void* arg) {
    websockets_protocol_resource_t* protocol = arg;

    protocol->method = ROUTE_NONE;
    protocol->parser_stage = WSPROTRESOURCE_METHOD;
    protocol->uri_length = 0;
    protocol->path_length = 0;
    protocol->ext_length = 0;

    if (protocol->uri) free((void*)protocol->uri);
    protocol->uri = NULL;

    if (protocol->path) free((void*)protocol->path);
    protocol->path = NULL;

    if (protocol->ext) free((void*)protocol->ext);
    protocol->ext = NULL;

    websocketsrequest_query_free(protocol->query_);
    protocol->query_ = NULL;
}

void websockets_protocol_resource_free(void* arg) {
    websockets_protocol_resource_t* protocol = arg;

    websockets_protocol_resource_reset(protocol);
    free(protocol);
}

int websocketsrequest_get_resource(connection_t* connection) {
    websocketsrequest_t* request = (websocketsrequest_t*)connection->request;
    websockets_protocol_resource_t* protocol = (websockets_protocol_resource_t*)request->protocol;

    if (protocol->method == ROUTE_NONE) return 0;
    if (protocol->path == NULL) return 0;

    for (route_t* route = connection->server->websockets.route; route; route = route->next) {
        if (route->is_primitive && route_compare_primitive(route, protocol->path, protocol->path_length)) {
            if (route->handler[protocol->method] == NULL) return 0;

            if (!websockets_queue_handler_add(connection, route->handler[protocol->method]))
                return 0;

            return 1;
        }

        int vector_size = route->params_count * 6;
        int vector[vector_size];

        // find resource by template
        int matches_count = pcre_exec(route->location, NULL, protocol->path, protocol->path_length, 0, 0, vector, vector_size);

        if (matches_count > 1) {
            int i = 1; // escape full string match

            websockets_query_t* last_query = websocketsrequest_last_query_item(protocol);

            for (route_param_t* param = route->param; param; param = param->next, i++) {
                size_t substring_length = vector[i * 2 + 1] - vector[i * 2];

                websockets_query_t* query = websockets_query_create(param->string, param->string_len, &protocol->path[vector[i * 2]], substring_length);

                if (query == NULL || query->key == NULL || query->value == NULL) return 0;

                if (last_query)
                    last_query->next = query;

                last_query = query;
            }

            if (route->handler[protocol->method] == NULL) return 0;

            if (!websockets_queue_handler_add(connection, route->handler[protocol->method]))
                return 0;

            return 1;
        }
    }

    return 0;
}

int websockets_protocol_resource_payload_parse(websocketsrequest_t* request, char* string, size_t length) {
    websockets_protocol_resource_t* protocol = (websockets_protocol_resource_t*)request->protocol;
    websocketsparser_t* parser = (websocketsparser_t*)request->parser;

    int end_frame = parser->payload_saved_length >= parser->frame.payload_length;
    int end_data = end_frame && parser->frame.fin;

    size_t offset = 0;
    for (size_t i = 0; i < length; i++) {
        string[i] ^= parser->frame.mask[parser->payload_index++ % 4];
        char ch = string[i];
        int last_symbol = i + 1 == length;
        int last_data = end_data && last_symbol;

        switch (protocol->parser_stage) {
        case WSPROTRESOURCE_METHOD:
            {
                size_t s = bufferdata_writed(&parser->buf);
                if (s > method_max_length)
                    return 0;

                if (ch != ' ')
                    bufferdata_push(&parser->buf, ch);

                if (ch == ' ' || last_data) {
                    protocol->parser_stage = WSPROTRESOURCE_LOCATION;

                    bufferdata_complete(&parser->buf);

                    char* value = bufferdata_get(&parser->buf);
                    size_t value_length = bufferdata_writed(&parser->buf);
                    if (!websocketsparser_parse_method(protocol, value, value_length))
                        return 0;

                    bufferdata_reset(&parser->buf);
                }

                if (last_data)
                    return 1;
            }
            break;
        case WSPROTRESOURCE_LOCATION:
            {
                if (ch != ' ')
                    bufferdata_push(&parser->buf, ch);

                if (ch == ' ' || last_symbol) {
                    bufferdata_complete(&parser->buf);

                    char* value = bufferdata_get(&parser->buf);
                    size_t value_length = bufferdata_writed(&parser->buf);

                    if (websocketsparser_append_uri(protocol, value, value_length) == -1)
                        return 0;

                    bufferdata_reset(&parser->buf);
                }

                if (ch == ' ' || last_data) {
                    protocol->parser_stage = WSPROTRESOURCE_DATA;
                    offset = i + 1;

                    if (!websocketsparser_parse_location(protocol, protocol->uri, protocol->uri_length))
                        return 0;
                }

                if (last_data)
                    return 1;
            }
            break;
        case WSPROTRESOURCE_DATA:
            break;
        }
    }

    if (protocol->parser_stage == WSPROTRESOURCE_DATA) {
        if (!websocketsrequest_has_payload(protocol))
            return 0;

        if (!websockets_create_tmpfile(request->protocol, config()->main.tmp))
            return 0;

        off_t payloadlength = lseek(request->protocol->payload.fd, 0, SEEK_END);
        if (payloadlength + length > config()->main.client_max_body_size)
            return 0;

        int r = write(request->protocol->payload.fd, &string[offset], length - offset);
        lseek(request->protocol->payload.fd, 0, SEEK_SET);
        if (r <= 0) return 0;
    }

    return 1;
}

const char* websocketsrequest_query(websockets_protocol_resource_t* protocol, const char* key) {
    if (key == NULL) return NULL;

    websockets_query_t* query = protocol->query_;

    while (query) {
        if (strcmp(key, query->key) == 0)
            return query->value;

        query = query->next;
    }

    return NULL;
}

websockets_query_t* websocketsrequest_last_query_item(websockets_protocol_resource_t* protocol) {
    websockets_query_t* query = protocol->query_;

    while (query) {
        if (query->next == NULL)
            return query;

        query = query->next;
    }

    return NULL;
}

void websocketsrequest_query_free(websockets_query_t* query) {
    while (query != NULL) {
        websockets_query_t* next = query->next;

        websockets_query_free(query);

        query = next;
    }
}

int websocketsparser_parse_method(websockets_protocol_resource_t* protocol, char* string, size_t length) {
    if (length == 3 && string[0] == 'G' && string[1] == 'E' && string[2] == 'T')
        protocol->method = ROUTE_GET;
    else if (length == 4 && string[0] == 'P' && string[1] == 'O' && string[2] == 'S' && string[3] == 'T')
        protocol->method = ROUTE_POST;
    else if (length == 5 && string[0] == 'P' && string[1] == 'A' && string[2] == 'T' && string[3] == 'C' && string[4] == 'H')
        protocol->method = ROUTE_PATCH;
    else if (length == 6 && string[0] == 'D' && string[1] == 'E' && string[2] == 'L' && string[3] == 'E' && string[4] == 'T' && string[5] == 'E')
        protocol->method = ROUTE_DELETE;
    else
        return 0;

    return 1;
}

int websocketsparser_parse_location(websockets_protocol_resource_t* protocol, char* string, size_t length) {
    if (string[0] != '/') return 0;

    size_t uri_point_end = 0;
    size_t path_point_end = 0;
    size_t ext_point_start = 0;

    for (size_t i = 0; i < length; i++) {
        char c = string[i];

        if (c == '?' && path_point_end == 0) {
            path_point_end = i;

            int result = websocketsparser_set_query(protocol, &string[i + 1], length - (i + 1));

            if (result == 0) goto next;
            if (result < 0) return result;
        }

        if (c == '.') {
            ext_point_start = i + 1;

            if (i + 1 < length && string[i] == '.' && string[i + 1] == '/') return 0;
            if (i + 2 < length && string[i + 1] == '.' && string[i + 2] == '/') return 0;
        }

        if (c == ' ') {
            uri_point_end = i;
            break;
        }
    }

    next:

    if (uri_point_end == 0) uri_point_end = length;
    if (path_point_end == 0) path_point_end = uri_point_end;
    if (ext_point_start == 0) ext_point_start = path_point_end;

    if (websocketsparser_set_path(protocol, string, path_point_end) == -1) return 0;
    if (websocketsparser_set_ext(protocol, &string[ext_point_start], path_point_end - ext_point_start) == -1) return 0;

    return 1;
}

int websocketsparser_append_uri(websockets_protocol_resource_t* protocol, const char* string, size_t length) {
    char* data = realloc(protocol->uri, protocol->uri_length + length + 1);
    if (data == NULL) return 0;

    protocol->uri = data;

    memcpy(&protocol->uri[protocol->uri_length], string, length);

    protocol->uri_length += length;
    protocol->uri[protocol->uri_length] = 0;

    if (protocol->uri[0] != '/') return -1;

    return 0;
}

int websocketsparser_set_path(websockets_protocol_resource_t* protocol, const char* string, size_t length) {
    if (string[0] != '/') return -1;

    protocol->path = (char*)malloc(length + 1);
    if (protocol->path == NULL) return -1;

    memcpy(protocol->path, string, length);

    protocol->path[length] = 0;
    protocol->path_length = length;

    return 0;
}

int websocketsparser_set_ext(websockets_protocol_resource_t* protocol, const char* string, size_t length) {
    protocol->ext = (char*)malloc(length + 1);
    if (protocol->ext == NULL) return -1;

    memcpy(protocol->ext, string, length);

    protocol->ext[length] = 0;
    protocol->ext_length = length;

    return 0;
}

int websocketsparser_set_query(websockets_protocol_resource_t* protocol, const char* string, size_t length) {
    size_t pos = 0;
    size_t point_start = 0;

    enum { KEY, VALUE } stage = KEY;

    websockets_query_t* query = websockets_query_create(NULL, 0, NULL, 0);

    if (query == NULL) return -1;

    protocol->query_ = query;

    websockets_query_t* last_query = query;

    for (; pos < length; pos++) {
        switch (string[pos]) {
        case '=':
            if (string[pos - 1] == '=') continue;

            stage = VALUE;

            query->key = websockets_set_field(&string[point_start], pos - point_start);

            if (query->key == NULL) return -1;

            point_start = pos + 1;
            break;
        case '&':
            stage = KEY;

            query->value = websockets_set_field(&string[point_start], pos - point_start);

            if (query->value == NULL) return -1;

            websockets_query_t* query_new = websockets_query_create(NULL, 0, NULL, 0);

            if (query_new == NULL) return -1;

            if (last_query)
                last_query->next = query_new;

            last_query = query_new;

            query = query_new;

            point_start = pos + 1;
            break;
        case '#':
            if (stage == KEY) {
                query->key = websockets_set_field(&string[point_start], pos - point_start);
                if (query->key == NULL) return -1;
            }
            else if (stage == VALUE) {
                query->value = websockets_set_field(&string[point_start], pos - point_start);
                if (query->value == NULL) return -1;
            }

            return 0;
        }
    }

    if (stage == KEY) {
        query->key = websockets_set_field(&string[point_start], pos - point_start);
        if (query->key == NULL) return -1;

        query->value = websockets_set_field("", 0);
        if (query->value == NULL) return -1;
    }
    else if (stage == VALUE) {
        query->value = websockets_set_field(&string[point_start], pos - point_start);
        if (query->value == NULL) return -1;
    }

    return 0;
}

int websocketsrequest_has_payload(websockets_protocol_resource_t* protocol) {
    switch (protocol->method) {
    case ROUTE_POST:
    case ROUTE_PATCH:
        return 1;
    default:
        break;
    }

    return 0;
}
