#include <string.h>
#include "../request/websocketsrequest.h"
#include "websocketscommon.h"
#include "websocketsparser.h"

int websocketsparser_parse_first_byte(websocketsparser_t*);
int websocketsparser_parse_second_byte(websocketsparser_t*);
int websocketsparser_parse_payload_length(websocketsparser_t*);
int websocketsparser_parse_mask(websocketsparser_t*);
int websocketsparser_parse_method(websocketsparser_t*);
int websocketsparser_parse_location(websocketsparser_t*);
int websocketsparser_parse_payload(websocketsparser_t*);
int websocketsparser_string_append(websocketsparser_t*);
int websocketsparser_set_payload_length(websocketsparser_t*, const unsigned char*, size_t);
int websocketsparser_set_method(websocketsrequest_t*, const char*, size_t);
int websocketsparser_set_location(websocketsrequest_t*, const char*, size_t);
int websocketsparser_set_uri(websocketsrequest_t*, const char*, size_t);
int websocketsparser_set_path(websocketsrequest_t*, const char*, size_t);
int websocketsparser_set_ext(websocketsrequest_t*, const char*, size_t);
int websocketsparser_set_payload(websocketsparser_t*, const char*, size_t);


void websocketsparser_init(websocketsparser_t* parser, websocketsrequest_t* request, char* buffer) {
    parser->stage = FIRST_BYTE;
    parser->mask_index = 0;
    parser->bytes_readed = 0;
    parser->pos_start = 0;
    parser->pos = 0;
    parser->string_len = 0;
    parser->decoded_index = 0;
    parser->string = NULL;
    parser->buffer = buffer;
    parser->request = request;

    websockets_frame_init(&parser->frame);
}

// void websocketsparser_free(websocketsparser_t* parser) {
//     if (parser->string) free(parser->string);
//     parser->string = NULL;
// }

void websocketsparser_set_bytes_readed(websocketsparser_t* parser, size_t bytes_readed) {
	parser->bytes_readed = bytes_readed;
}

int websocketsparser_run(websocketsparser_t* parser) {
    int result = 0;

    parser->pos_start = 0;
    parser->pos = 0;

    if (parser->stage == FIRST_BYTE) {
        result = websocketsparser_parse_first_byte(parser);
        if (result < 0) return result;
    }
    if (parser->stage == SECOND_BYTE) {
        result = websocketsparser_parse_second_byte(parser);
        if (result < 0) return result;
    }
    if (parser->stage == PAYLOAD_LEN) {
        result = websocketsparser_parse_payload_length(parser);
        if (result < 0) return result;
    }
    if (parser->stage == MASK_KEY) {
        result = websocketsparser_parse_mask(parser);
        if (result < 0) return result;
    }
    if (parser->frame.payload_length == 0) {
        return 0;
    }

    // int is_custom_payload_parser = 0;

    // if (is_custom_payload_parser == 0) {
        if (parser->stage == METHOD) {
            result = websocketsparser_parse_method(parser);
            if (result < 0) return result;
        }
        if (parser->stage == LOCATION) {
            result = websocketsparser_parse_location(parser);
            if (result < 0) return result;
        }
    // }
    if (parser->stage == DATA) {
        result = websocketsparser_parse_payload(parser);
        if (result < 0) return result;
    }

    return result;
}

int websocketsparser_parse_first_byte(websocketsparser_t* parser) {
    unsigned char c = parser->buffer[0];

    parser->frame.fin = (c >> 7) & 0x01;
    parser->frame.rsv1 = (c >> 6) & 0x01;
    parser->frame.rsv2 = (c >> 5) & 0x01;
    parser->frame.rsv3 = (c >> 4) & 0x01;
    parser->frame.opcode = c & 0x0F;

    if (parser->request->type == NONE && parser->frame.opcode == 1) parser->request->type = WEBSOCKETS_TEXT;
    if (parser->request->type == NONE && parser->frame.opcode == 2) parser->request->type = WEBSOCKETS_BINARY;

    parser->stage = SECOND_BYTE;

    parser->pos_start++;

    if (parser->pos + 1 == parser->bytes_readed) return -2;

    return 0;
}

int websocketsparser_parse_second_byte(websocketsparser_t* parser) {
    if (parser->pos_start > parser->bytes_readed) return -1;

    parser->pos = parser->pos_start;

    unsigned char c = parser->buffer[parser->pos_start];

    parser->frame.masked = (c >> 7) & 0x01;
    parser->frame.payload_length = c & (~0x80);

    parser->pos_start = parser->pos + 1;

    if (parser->frame.payload_length <= 125) {
        // parser->stage = parser->frame.masked ? MASK_KEY : is_custom_payload_parser == 0 ? METHOD : DATA;
        parser->stage = parser->frame.masked ? MASK_KEY : METHOD;
    }
    else {
        parser->stage = PAYLOAD_LEN;
    }

    if (parser->pos + 1 == parser->bytes_readed) return -2;

    return 0;
}

int websocketsparser_parse_payload_length(websocketsparser_t* parser) {
    for (parser->pos = parser->pos_start; parser->pos < parser->bytes_readed; parser->pos++) {
        int payload_string_length = parser->string_len + (parser->pos - parser->pos_start);

        if (parser->frame.payload_length == 126 && payload_string_length == 2) goto next;
        if (parser->frame.payload_length == 127 && payload_string_length == 8) goto next;
    }

    return websocketsparser_string_append(parser);

    next:

    if (parser->string && websocketsparser_string_append(parser) == -1) return -1;

    if (parser->string) {
        if (websocketsparser_set_payload_length(parser, parser->string, parser->string_len) == -1) return -1;
    } else {
        if (websocketsparser_set_payload_length(parser, &parser->buffer[parser->pos_start], parser->pos - parser->pos_start) == -1) return -1;
    }

    parser->pos_start = parser->pos;

    if (parser->string) {
        free(parser->string);
        parser->string = NULL;
    }

    parser->string_len = 0;

    // parser->stage = parser->frame.masked ? MASK_KEY : is_custom_payload_parser == 0 ? METHOD : DATA;
    parser->stage = parser->frame.masked ? MASK_KEY : METHOD;

    if (parser->pos == parser->bytes_readed) return -2;

    return 0;
}

int websocketsparser_parse_mask(websocketsparser_t* parser) {
    for (parser->pos = parser->pos_start; parser->pos < parser->bytes_readed; parser->pos++) {
        parser->frame.mask[parser->mask_index] = parser->buffer[parser->pos];
        parser->mask_index++;

        if (parser->mask_index == 4) goto next;
    }

    return -2;

    next:

    parser->pos_start = parser->pos + 1;

    if (parser->string) {
        free(parser->string);
        parser->string = NULL;
    }

    parser->string_len = 0;

    // int is_custom_payload_parser = 0;

    // if (is_custom_payload_parser == 0) {
        parser->stage = METHOD;
    // }
    // else {
    //     parser->stage = DATA;
    // }

    if (parser->pos + 1 == parser->bytes_readed) return -2;

    return 0;
}

int websocketsparser_parse_method(websocketsparser_t* parser) {
    size_t method_max_length = 6;

    for (parser->pos = parser->pos_start; parser->pos < parser->bytes_readed; parser->pos++) {
        if (parser->frame.masked) {
            parser->buffer[parser->pos] = (parser->buffer[parser->pos]) ^ parser->frame.mask[parser->decoded_index++ % 4];
        }

        if (parser->buffer[parser->pos] == ' ') goto next;
        if (parser->pos - parser->pos_start > method_max_length) goto next;
    }

    return websocketsparser_string_append(parser);

    next:

    if (parser->string && websocketsparser_string_append(parser) == -1) return -1;

    if (parser->string) {
        if (websocketsparser_set_method(parser->request, parser->string, parser->string_len) == -1) return -1;
    } else {
        if (websocketsparser_set_method(parser->request, &parser->buffer[parser->pos_start], parser->pos - parser->pos_start) == -1) return -1;
    }

    parser->pos_start = parser->pos + 1;

    if (parser->string) {
        free(parser->string);
        parser->string = NULL;
    }

    parser->string_len = 0;

    parser->stage = LOCATION;

    if (parser->pos + 1 == parser->bytes_readed) return -2;

    return 0;
}

int websocketsparser_parse_location(websocketsparser_t* parser) {
    size_t uri_max_length = 8196;

    for (parser->pos = parser->pos_start; parser->pos < parser->bytes_readed; parser->pos++) {
        if (parser->frame.masked) {
            parser->buffer[parser->pos] = (parser->buffer[parser->pos]) ^ parser->frame.mask[parser->decoded_index++ % 4];
        }

        if (parser->buffer[parser->pos] == ' ') goto next;
        if (parser->pos - parser->pos_start > uri_max_length) goto next;
    }

    return websocketsparser_string_append(parser);

    next:

    if (parser->string && websocketsparser_string_append(parser) == -1) return -1;

    if (parser->string) {
        if (websocketsparser_set_location(parser->request, parser->string, parser->string_len) == -1) return -1;
    } else {
        if (websocketsparser_set_location(parser->request, &parser->buffer[parser->pos_start], parser->pos - parser->pos_start) == -1) return -1;
    }

    parser->pos_start = parser->pos + 1;

    if (parser->string) {
        free(parser->string);
        parser->string = NULL;
    }

    parser->string_len = 0;

    parser->stage = DATA;

    if (parser->pos + 1 == parser->bytes_readed) return -2;

    return 0;
}

int websocketsparser_parse_payload(websocketsparser_t* parser) {
    if (parser->frame.masked) {
        for (parser->pos = parser->pos_start; parser->pos < parser->bytes_readed; parser->pos++) {
            parser->buffer[parser->pos] = (parser->buffer[parser->pos]) ^ parser->frame.mask[parser->decoded_index++ % 4];
        }
    }
    else {
        parser->pos = parser->bytes_readed;
    }

    if (parser->pos - parser->pos_start == parser->frame.payload_length) goto next;

    return websocketsparser_string_append(parser);

    next:

    if (parser->string && websocketsparser_string_append(parser) == -1) return -1;

    if (parser->string) {
        if (websocketsparser_set_payload(parser, parser->string, parser->string_len) == -1) return -1;
    } else {
        if (websocketsparser_set_payload(parser, &parser->buffer[parser->pos_start], parser->pos - parser->pos_start) == -1) return -1;
    }

    if (parser->string) {
        free(parser->string);
        parser->string = NULL;
    }

    parser->string_len = 0;

    if (parser->pos == parser->bytes_readed) return -2;

    return 0;
}

int websocketsparser_string_append(websocketsparser_t* parser) {
    size_t string_len = parser->pos - parser->pos_start;

    if (parser->string == NULL) {
        parser->string_len = string_len;

        parser->string = (char*)malloc(parser->string_len + 1);

        if (parser->string == NULL) return -1;

        memcpy(parser->string, &parser->buffer[parser->pos_start], parser->string_len);
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
            memcpy(&parser->string[len], &parser->buffer[parser->pos_start], string_len);
        }
    }

    parser->string[parser->string_len] = 0;

    return -2;
}

int websocketsparser_set_payload_length(websocketsparser_t* parser, const unsigned char* string, size_t length) {
    int byte_count = 0;
    
    if (parser->frame.payload_length == 126) {
        // parser->frame.payload_length = (
        //     (string[0] << 8) | 
        //     (string[1])
        // );

        byte_count = 2;
    }
    else if (parser->frame.payload_length == 127) {
        // parser->frame.payload_length = (
        //     (string[0] << 56) |
        //     (string[1] << 48) |
        //     (string[2] << 40) |
        //     (string[3] << 32) |
        //     (string[4] << 24) |
        //     (string[5] << 16) |
        //     (string[6] << 8)  |
        //     (string[7])
        // );

        byte_count = 8;
    }
    else {
        return -1;
    }

    parser->frame.payload_length = 0;

    int counter = byte_count;
    int byte_left = 8;

    do {
        parser->frame.payload_length |= (string[byte_count - counter]) << (byte_left * counter - byte_left);
    } while (--counter > 0);

    return 0;
}

int websocketsparser_save_payload(websocketsparser_t* parser, websocketsrequest_t* request) {
    if (parser->frame.payload_length == 0) return 0;

    int result = 0;
    // int is_custom_payload_parser = 0;

    // if (is_custom_payload_parser == 0) {
        if (parser->string && parser->stage == METHOD) {
            result = websocketsparser_set_method(parser->request, parser->string, parser->string_len);
        }
        if (parser->string && parser->stage == LOCATION) {
            result = websocketsparser_set_location(parser->request, parser->string, parser->string_len);
        }
    // }
    if (parser->string && parser->stage == DATA) {
        result = websocketsparser_set_payload(parser, parser->string, parser->string_len);
    }

    if (parser->string) {
        free(parser->string);
        parser->string = NULL;
    }

    parser->string_len = 0;

    return result;
}

int websocketsparser_set_method(websocketsrequest_t* request, const char* string, size_t length) {
    if (length == 3 && string[0] == 'G' && string[1] == 'E' && string[2] == 'T') {
        request->method = ROUTE_GET;
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
    else return -1;

    return 0;
}

int websocketsparser_set_location(websocketsrequest_t* request, const char* string, size_t length) {
    if (string[0] != '/') return -1;

    char prev_c = 0;
    size_t uri_point_end = 0;
    size_t path_point_end = 0;
    size_t ext_point_start = 0;

    for (size_t i = 0; i < length; i++) {
        char c = string[i];

        if (c == '?' && path_point_end == 0) {
            path_point_end = i;
        }

        if (c == '.') {
            ext_point_start = i + 1;

            if (i + 2 < length && string[i + 1] == '.' && string[i + 2] == '/') return -1;
        }

        if (c == ' ') {
            uri_point_end = i;
            break;
        }

        prev_c = c;
    }

    if (uri_point_end == 0) uri_point_end = length;

    if (path_point_end == 0) path_point_end = uri_point_end;

    if (ext_point_start == 0) ext_point_start = path_point_end;

    if (websocketsparser_set_uri(request, string, length) == -1) return -1;

    if (websocketsparser_set_path(request, string, path_point_end) == -1) return -1;

    if (websocketsparser_set_ext(request, &string[ext_point_start], path_point_end - ext_point_start) == -1) return -1;

    printf("uri %s\n", request->uri);
    printf("path %s\n", request->path);
    printf("ext %s\n", request->ext);

    return 0;
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

int websocketsparser_set_payload(websocketsparser_t* parser, const char* string, size_t length) {
    websocketsrequest_t* request = parser->request;

    if (request->payload == NULL) {
        request->payload_length = length;

        request->payload = (char*)malloc(length + 1);

        if (request->payload == NULL) return -1;

        memcpy(request->payload, string, length);
    } else {
        size_t len = length;
        request->payload_length += length;

        char* data = (char*)realloc(request->payload, request->payload_length + 1);

        if (data == NULL) {
            free(request->payload);
            request->payload = NULL;
            return -1;
        }

        request->payload = data;

        if (request->payload_length > len) {
            memcpy(&request->payload[len], string, length);
        }
    }

    request->payload[request->payload_length] = 0;

    if (request->payload_length < parser->frame.payload_length) return -2;

    return 0;
}