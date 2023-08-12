#include <string.h>

#include "websocketsprotocolresource.h"

static const size_t method_max_length = 6;

int websocketsrequest_get_resource(connection_t*);
void websockets_protocol_resource_reset(websockets_protocol_resource_t*);
void websockets_protocol_resource_free(websockets_protocol_resource_t*);
const char* websocketsrequest_query(websockets_protocol_resource_t*, const char*);

websockets_protocol_resource_t* websockets_protocol_resource_create() {
    websockets_protocol_resource_t* protocol = malloc(sizeof * protocol);
    if (protocol == NULL) return NULL;

    protocol->base.get_resource = websocketsrequest_get_resource;
    protocol->base.reset = websockets_protocol_resource_reset;
    protocol->base.free = websockets_protocol_resource_free;
    protocol->payload = NULL;
    protocol->payloadf = NULL;
    protocol->payload_json = NULL;
    protocol->method = ROUTE_NONE;
    protocol->uri_length = 0;
    protocol->path_length = 0;
    protocol->ext_length = 0;
    protocol->uri = NULL;
    protocol->path = NULL;
    protocol->ext = NULL;
    protocol->query_ = NULL;
    protocol->query = websocketsrequest_query;

    return protocol;
}

void websockets_protocol_resource_reset(websockets_protocol_resource_t* protocol) {
    if (protocol->uri) free((void*)protocol->uri);
    protocol->uri = NULL;

    if (protocol->path) free((void*)protocol->path);
    protocol->path = NULL;

    if (protocol->ext) free((void*)protocol->ext);
    protocol->ext = NULL;

    websocketsrequest_query_free(protocol->query_);
    protocol->query_ = NULL;
}

void websockets_protocol_resource_free(websockets_protocol_resource_t* protocol) {
    websockets_protocol_resource_reset(protocol);
    free(protocol);
}

int websocketsrequest_get_resource(connection_t* connection) {
    websocketsrequest_t* request = (websocketsrequest_t*)connection->request;
    websockets_protocol_resource_t* protocol = (websockets_protocol_resource_t*)request->protocol;

    if (protocol->method == ROUTE_NONE) return -1;
    if (protocol->path == NULL) return -1;

    for (route_t* route = connection->server->websockets.route; route; route = route->next) {
        if (route->is_primitive && route_compare_primitive(route, protocol->path, protocol->path_length)) {
            if (route->handler[protocol->method] == NULL) return -1;

            connection->handle = route->handler[protocol->method];
            connection->queue_push(connection);
            return 1;
        }

        int vector_size = route->params_count * 6;
        int vector[vector_size];

        // find resource by template
        int matches_count = pcre_exec(route->location, NULL, protocol->path, protocol->path_length, 0, 0, vector, vector_size);

        if (matches_count > 1) {
            int i = 1; // escape full string match

            websockets_query_t* last_query = NULL;

            for (route_param_t* param = route->param; param; param = param->next, i++) {
                size_t substring_length = vector[i * 2 + 1] - vector[i * 2];

                websockets_query_t* query = websockets_query_create(param->string, param->string_len, &protocol->path[vector[i * 2]], substring_length);

                if (query == NULL || query->key == NULL || query->value == NULL) return -1;

                if (last_query)
                    last_query->next = query;

                last_query = query;
            }

            if (route->handler[protocol->method] == NULL) return -1;

            connection->handle = route->handler[protocol->method];
            connection->queue_push(connection);
            return 1;
        }
    }

    return 0;
}

int websockets_protocol_resource_payload_parse(websocketsrequest_t* request, const char* string, size_t length, int end) {
    return 0;
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

void websocketsrequest_query_free(websockets_query_t* query) {
    while (query != NULL) {
        websockets_query_t* next = query->next;

        websockets_query_free(query);

        query = next;
    }
}

int websocketsparser_parse_method(websocketsparser_t* parser) {
    websocketsrequest_t* request = (websocketsrequest_t*)parser->connection->request;
    char* string = websocketsparser_buffer_get(parser);
    size_t length = websocketsparser_buffer_writed(parser);

    if (length == 3 && string[0] == 'G' && string[1] == 'E' && string[2] == 'T')
        request->method = ROUTE_GET;
    else if (length == 4 && string[0] == 'P' && string[1] == 'O' && string[2] == 'S' && string[3] == 'T')
        request->method = ROUTE_POST;
    else if (length == 5 && string[0] == 'P' && string[1] == 'A' && string[2] == 'T' && string[3] == 'C' && string[4] == 'H')
        request->method = ROUTE_PATCH;
    else if (length == 6 && string[0] == 'D' && string[1] == 'E' && string[2] == 'L' && string[3] == 'E' && string[4] == 'T' && string[5] == 'E')
        request->method = ROUTE_DELETE;
    else
        return 0;

    return 1;
}

int websocketsparser_parse_location(websocketsparser_t* parser) {
    websocketsrequest_t* request = (websocketsrequest_t*)parser->connection->request;
    char* string = websocketsparser_buffer_get(parser);
    size_t length = websocketsparser_buffer_writed(parser);

    if (string[0] != '/') return 0;

    size_t uri_point_end = 0;
    size_t path_point_end = 0;
    size_t ext_point_start = 0;

    for (size_t i = 0; i < length; i++) {
        char c = string[i];

        if (c == '?' && path_point_end == 0) {
            path_point_end = i;

            int result = websocketsparser_set_query(request, &string[i + 1], length - (i + 1));

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

    if (websocketsparser_set_uri(request, string, length) == -1) return 0;
    if (websocketsparser_set_path(request, string, path_point_end) == -1) return 0;
    if (websocketsparser_set_ext(request, &string[ext_point_start], path_point_end - ext_point_start) == -1) return 0;

    return 1;
}


int websocketsparser_set_uri(websocketsrequest_t* request, const char* string, size_t length) {
    if (string[0] != '/') return -1;

    request->uri = (char*)malloc(length + 1);

    if (request->uri == NULL) return -1;

    memcpy(request->uri, string, length);

    request->uri[length] = 0;

    request->uri_length = length;

    return 0;
}

int websocketsparser_set_path(websocketsrequest_t* request, const char* string, size_t length) {
    if (string[0] != '/') return -1;

    request->path = (char*)malloc(length + 1);

    if (request->path == NULL) return -1;

    memcpy(request->path, string, length);

    request->path[length] = 0;

    request->path_length = length;

    return 0;
}

int websocketsparser_set_ext(websocketsrequest_t* request, const char* string, size_t length) {
    request->ext = (char*)malloc(length + 1);

    if (request->ext == NULL) return -1;

    memcpy(request->ext, string, length);

    request->ext[length] = 0;

    request->ext_length = length;

    return 0;
}

int websocketsparser_set_query(websocketsrequest_t* request, const char* string, size_t length) {
    size_t pos = 0;
    size_t point_start = 0;

    enum { KEY, VALUE } stage = KEY;

    websockets_query_t* query = websockets_query_create(NULL, 0, NULL, 0);

    if (query == NULL) return -1;

    request->query_ = query;
    request->last_query = query;

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

            websocketsparser_append_query(request, query_new);

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

int websocketsrequest_has_payload(websocketsrequest_t* request) {
    switch (request->method) {
    case ROUTE_POST:
    case ROUTE_PATCH:
        return 1;
    default:
        break;
    }

    return 0;
}

int websocketsparser_parse_payload(websocketsparser_t* parser) {
    websocketsrequest_t* request = (websocketsrequest_t*)parser->connection->request;

    // if (!websocketsrequest_has_payload(request))
    //     return 0;

    size_t pos_start = parser->pos_start = parser->pos;
    parser->pos = parser->bytes_readed;

    for (; pos_start < parser->bytes_readed; pos_start++) {
        char ch = parser->buffer[pos_start];
        parser->buffer[pos_start] = ch ^ parser->frame.mask[parser->payload_index++ % 4];


        // switch (parser->payload_stage) {
        // case WSPAYLOAD_METHOD:
        //     {
        //         size_t s = websocketsparser_buffer_writed(parser);

        //         if (s > method_max_length)
        //             return WSPARSER_BAD_REQUEST;

        //         if (ch != ' ')
        //             websocketsparser_buffer_push(parser, ch);

        //         if (ch == ' ' || (end && parser->frame.fin == 1)) {
        //             parser->payload_stage = WSPAYLOAD_LOCATION;

        //             websocketsparser_buffer_complete(parser);
        //             if (!websocketsparser_parse_method(parser))
        //                 return WSPARSER_BAD_REQUEST;

        //             websocketsparser_buffer_reset(parser);
        //         }

        //         if (end) {
        //             if (parser->frame.fin)
        //                 return WSPARSER_COMPLETE;
        //             else
        //                 return WSPARSER_CONTINUE;
        //         }
        //     }
        //     break;
        // case WSPAYLOAD_LOCATION:
        //     {
        //         if (ch != ' ')
        //             websocketsparser_buffer_push(parser, ch);

        //         if (end) {
        //             printf("end");
        //         }

        //         if (ch == ' ' || (end && parser->frame.fin == 1)) {
        //             parser->payload_stage = WSPAYLOAD_DATA;

        //             websocketsparser_buffer_complete(parser);
        //             if (!websocketsparser_parse_location(parser))
        //                 return WSPARSER_BAD_REQUEST;

        //             websocketsparser_buffer_reset(parser);
        //         }

        //         if (end) {
        //             return WSPARSER_COMPLETE;
        //         }
        //     }
        //     break;
        // case WSPAYLOAD_DATA:
            
        // }
    }

    // size_t string_len = parser->pos - parser->pos_start;
    // parser->payload_saved_length += string_len;

    // int end = parser->payload_saved_length >= parser->frame.payload_length && parser->frame.fin;

    // if (!request->payload_parser(request, &parser->buffer[pos_start], string_len, end)) return WSPARSER_ERROR;

    // return end ? WSPARSER_COMPLETE : WSPARSER_CONTINUE;



    if (request->payload.fd <= 0) {
        const char* template = "tmp.XXXXXX";
        const char* tmp_dir = parser->connection->server->info->tmp_dir;

        size_t path_length = strlen(tmp_dir) + strlen(template) + 2; // "/", "\0"
        request->payload.path = malloc(path_length);
        snprintf(request->payload.path, path_length, "%s/%s", tmp_dir, template);

        request->payload.fd = mkstemp(request->payload.path);
        if (request->payload.fd == -1) return 0;
    }

    off_t payloadlength = lseek(request->payload.fd, 0, SEEK_END);

    if (payloadlength + string_len > parser->connection->server->info->client_max_body_size) {
        return 0;
    }

    int r = write(request->payload.fd, &parser->buffer[parser->pos_start], string_len);
    lseek(request->payload.fd, 0, SEEK_SET);
    if (r <= 0) return 0;

    parser->payload_saved_length += string_len;

    if (parser->payload_saved_length >= parser->frame.payload_length)
        return WSPARSER_COMPLETE;

    return WSPARSER_CONTINUE;
}