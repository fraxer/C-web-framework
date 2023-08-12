#include <string.h>

#include "websocketsrequest.h"
#include "websocketscommon.h"
#include "websocketsparser.h"

int websocketsparser_parse_first_byte(websocketsparser_t*);
int websocketsparser_parse_second_byte(websocketsparser_t*);
int websocketsparser_parse_payload_length_126(websocketsparser_t*);
int websocketsparser_parse_payload_length_127(websocketsparser_t*);
int websocketsparser_parse_mask(websocketsparser_t*);
int websocketsparser_parse_method(websocketsparser_t*);
int websocketsparser_parse_location(websocketsparser_t*);
int websocketsparser_parse_payload(websocketsparser_t*);
int websocketsparser_string_append(websocketsparser_t*);
int websocketsparser_set_payload_length(websocketsparser_t*, int);
int websocketsparser_set_control_payload(websocketsparser_t*, const char*, size_t);
int websocketsparser_set_payload(websocketsparser_t*, const char*, size_t);
void websocketsparser_flush(websocketsparser_t*);
void websockets_buffer_init(websocketsparser_buffer_t*);
int websocketsparser_is_control_frame(websockets_frame_t*);


void websocketsparser_init(websocketsparser_t* parser) {
    parser->stage = WSPARSER_FIRST_BYTE;
    parser->mask_index = 0;
    parser->bytes_readed = 0;
    parser->pos_start = 0;
    parser->pos = 0;
    parser->payload_index = 0;
    parser->buffer = NULL;
    parser->connection = NULL;
    parser->payload_saved_length = 0;

    websockets_frame_init(&parser->frame);
    websockets_buffer_init(&parser->buf);
}

void websocketsparser_set_connection(websocketsparser_t* parser, connection_t* connection) {
    parser->connection = connection;
}

void websocketsparser_set_buffer(websocketsparser_t* parser, char* buffer) {
    parser->buffer = buffer;
}

void websocketsparser_free(websocketsparser_t* parser) {
    websocketsparser_flush(parser);
    free(parser);
}

void websocketsparser_reset(websocketsparser_t* parser) {
    websocketsparser_flush(parser);
    websocketsparser_init(parser);
}

void websocketsparser_flush(websocketsparser_t* parser) {
    if (parser->buf.dynamic_buffer) free(parser->buf.dynamic_buffer);
    parser->buf.dynamic_buffer = NULL;
}

void websocketsparser_set_bytes_readed(websocketsparser_t* parser, size_t bytes_readed) {
	parser->bytes_readed = bytes_readed;
}

int websocketsparser_run(websocketsparser_t* parser) {
    parser->pos_start = 0;
    parser->pos = 0;

    if (parser->stage == WSPARSER_PAYLOAD)
        return websocketsparser_parse_payload(parser);

    for (parser->pos = parser->pos_start; parser->pos < parser->bytes_readed; parser->pos++) {
        char ch = parser->buffer[parser->pos];

        switch (parser->stage)
        {
        case WSPARSER_FIRST_BYTE:
            {
                websocketsparser_buffer_push(parser, ch);

                parser->stage = WSPARSER_SECOND_BYTE;

                websocketsparser_buffer_complete(parser);
                if (!websocketsparser_parse_first_byte(parser))
                    return WSPARSER_BAD_REQUEST;

                websocketsparser_buffer_reset(parser);
            }
            break;
        case WSPARSER_SECOND_BYTE:
            {
                websocketsparser_buffer_push(parser, ch);

                websocketsparser_buffer_complete(parser);
                if (!websocketsparser_parse_second_byte(parser))
                    return WSPARSER_BAD_REQUEST;

                else if (parser->frame.payload_length == 126) {
                    parser->stage = WSPARSER_PAYLOAD_LEN_126;
                }
                else if (parser->frame.payload_length == 127) {
                    parser->stage = WSPARSER_PAYLOAD_LEN_127;
                }
                else {
                    parser->stage = WSPARSER_MASK_KEY;
                }

                websocketsparser_buffer_reset(parser);
            }
            break;
        case WSPARSER_PAYLOAD_LEN_126:
            {
                if (websocketsparser_is_control_frame(&parser->frame))
                    return WSPARSER_BAD_REQUEST;

                websocketsparser_buffer_push(parser, ch);

                if (websocketsparser_buffer_writed(parser) < 2)
                    break;

                parser->stage = WSPARSER_MASK_KEY;

                websocketsparser_buffer_complete(parser);
                if (!websocketsparser_parse_payload_length_126(parser))
                    return WSPARSER_BAD_REQUEST;

                websocketsparser_buffer_reset(parser);
            }
            break;
        case WSPARSER_PAYLOAD_LEN_127:
            {
                if (websocketsparser_is_control_frame(&parser->frame))
                    return WSPARSER_BAD_REQUEST;

                websocketsparser_buffer_push(parser, ch);

                if (websocketsparser_buffer_writed(parser) < 8)
                    break;

                parser->stage = WSPARSER_MASK_KEY;

                websocketsparser_buffer_complete(parser);
                if (!websocketsparser_parse_payload_length_127(parser))
                    return WSPARSER_BAD_REQUEST;

                websocketsparser_buffer_reset(parser);
            }
            break;
        case WSPARSER_MASK_KEY:
            {
                websocketsparser_buffer_push(parser, ch);

                if (websocketsparser_buffer_writed(parser) < 4)
                    break;

                websocketsparser_buffer_complete(parser);
                if (!websocketsparser_parse_mask(parser))
                    return WSPARSER_BAD_REQUEST;

                websocketsparser_buffer_reset(parser);

                if (parser->frame.payload_length == 0)
                    return WSPARSER_COMPLETE;
            }
            break;
        case WSPARSER_CONTROL_PAYLOAD:
            {
                parser->payload_saved_length++;
                int end = parser->payload_saved_length == parser->frame.payload_length;

                ch = ch ^ parser->frame.mask[parser->payload_index++ % 4];

                websocketsparser_buffer_push(parser, ch);

                if (end) {
                    websocketsparser_buffer_complete(parser);
                    return WSPARSER_COMPLETE;
                }
            }
            break;
        case WSPARSER_PAYLOAD:
            return websocketsparser_parse_payload(parser);
        default:
            break;
        }
    }

    return WSPARSER_CONTINUE;
}

int websocketsparser_parse_first_byte(websocketsparser_t* parser) {
    char* string = websocketsparser_buffer_get(parser);
    unsigned char c = string[0];

    parser->frame.fin = (c >> 7) & 0x01;
    parser->frame.rsv1 = (c >> 6) & 0x01;
    parser->frame.rsv2 = (c >> 5) & 0x01;
    parser->frame.rsv3 = (c >> 4) & 0x01;
    parser->frame.opcode = c & 0x0F;

    websocketsrequest_t* request = (websocketsrequest_t*)parser->connection->request;
    if (!parser->frame.fin)
        request->fragmented = 1;

    if (request->type == WEBSOCKETS_NONE) {
        request->type = parser->frame.opcode + 0x80;
    }

    if (websocketsparser_is_control_frame(&parser->frame) && (request->type == WEBSOCKETS_TEXT || request->type == WEBSOCKETS_BINARY)) {
        request->control_type = WEBSOCKETS_CONTINUE;
    }
    if (parser->frame.fin == 0 || parser->frame.opcode == WSOPCODE_CONTINUE) {
        request->control_type = WEBSOCKETS_CONTINUE;
    }

    return 1;
}

int websocketsparser_parse_second_byte(websocketsparser_t* parser) {
    char* string = websocketsparser_buffer_get(parser);
    unsigned char c = string[0];

    parser->frame.masked = (c >> 7) & 0x01;
    parser->frame.payload_length = c & (~0x80);

    if (!parser->frame.masked) return 0;

    return 1;
}

int websocketsparser_parse_payload_length_126(websocketsparser_t* parser) {
    int byte_count = 2;
    return websocketsparser_set_payload_length(parser, byte_count);
}

int websocketsparser_parse_payload_length_127(websocketsparser_t* parser) {
    int byte_count = 8;
    return websocketsparser_set_payload_length(parser, byte_count);
}

int websocketsparser_set_payload_length(websocketsparser_t* parser, int byte_count) {
    parser->frame.payload_length = 0;

    int counter = byte_count;
    const int byte_left = 8;
    const unsigned char* value = (const unsigned char*)websocketsparser_buffer_get(parser);

    do {
        parser->frame.payload_length |= value[byte_count - counter] << (byte_left * counter - byte_left);
    } while (--counter > 0);

    return 1;
}

int websocketsparser_parse_mask(websocketsparser_t* parser) {
    char* string = websocketsparser_buffer_get(parser);

    for (size_t i = 0; i < websocketsparser_buffer_writed(parser); i++) 
        parser->frame.mask[i] = string[i];

    if (websocketsparser_is_control_frame(&parser->frame))
        parser->stage = WSPARSER_CONTROL_PAYLOAD;
    else
        parser->stage = WSPARSER_PAYLOAD;

    return 1;
}

int websocketsparser_parse_payload(websocketsparser_t* parser) {
    websocketsrequest_t* request = (websocketsrequest_t*)parser->connection->request;

    size_t pos_start = parser->pos;
    size_t size = parser->bytes_readed - pos_start;

    int end = parser->payload_saved_length >= parser->frame.payload_length && parser->frame.fin;

    if (!request->protocol->payload_parse(request, &parser->buffer[pos_start], size, end)) return WSPARSER_ERROR;

    return end ? WSPARSER_COMPLETE : WSPARSER_CONTINUE;
}

void websockets_buffer_init(websocketsparser_buffer_t* buffer) {
    buffer->dynamic_buffer = NULL;
    buffer->offset_sbuffer = 0;
    buffer->offset_dbuffer = 0;
    buffer->dbuffer_size = 0;
    buffer->type = STATIC;
}

int websocketsparser_is_control_frame(websockets_frame_t* frame)
{
    switch (frame->opcode) {
    case WSOPCODE_CLOSE:
    case WSOPCODE_PING:
    case WSOPCODE_PONG:
        return 1;
    }

    return 0;
}

int websocketsparser_buffer_push(websocketsparser_t* parser, char ch) {
    parser->buf.static_buffer[parser->buf.offset_sbuffer] = ch;
    parser->buf.offset_sbuffer++;

    if (parser->buf.offset_sbuffer < WSPARSER_BUFSIZ) {
        parser->buf.static_buffer[parser->buf.offset_sbuffer] = 0;
        return WSPARSER_CONTINUE;
    }

    parser->buf.type = DYNAMIC;

    return websocketsparser_buffer_move(parser);
}

void websocketsparser_buffer_reset(websocketsparser_t* parser) {
    parser->buf.offset_dbuffer = 0;
    parser->buf.offset_sbuffer = 0;
    parser->buf.type = STATIC;
}

size_t websocketsparser_buffer_writed(websocketsparser_t* parser) {
    if (parser->buf.type == DYNAMIC) {
        return parser->buf.offset_dbuffer + parser->buf.offset_sbuffer;
    }
    return parser->buf.offset_sbuffer;
}

int websocketsparser_buffer_complete(websocketsparser_t* parser) {
    if (parser->buf.type == DYNAMIC) {
        return websocketsparser_buffer_move(parser);
    }
    return WSPARSER_CONTINUE;
}

int websocketsparser_buffer_move(websocketsparser_t* parser) {
    if (parser->buf.type != DYNAMIC)
        return WSPARSER_CONTINUE;

    size_t dbuffer_length = parser->buf.offset_dbuffer + parser->buf.offset_sbuffer;

    if (parser->buf.dbuffer_size <= dbuffer_length) {
        char* data = realloc(parser->buf.dynamic_buffer, dbuffer_length + 1);

        if (data == NULL)
            return WSPARSER_OUT_OF_MEMORY;

        parser->buf.dbuffer_size = dbuffer_length + 1;
        parser->buf.dynamic_buffer = data;
    }

    memcpy(&parser->buf.dynamic_buffer[parser->buf.offset_dbuffer], parser->buf.static_buffer, parser->buf.offset_sbuffer);

    parser->buf.dynamic_buffer[dbuffer_length] = 0;
    parser->buf.offset_dbuffer = dbuffer_length;
    parser->buf.offset_sbuffer = 0;

    return WSPARSER_CONTINUE;
}

char* websocketsparser_buffer_get(websocketsparser_t* parser) {
    if (parser->buf.type == DYNAMIC) {
        return parser->buf.dynamic_buffer;
    }
    return parser->buf.static_buffer;
}

char* websocketsparser_buffer_copy(websocketsparser_t* parser) {
    char* string = websocketsparser_buffer_get(parser);
    size_t length = websocketsparser_buffer_writed(parser);

    char* data = malloc(length + 1);
    if (data == NULL) return NULL;

    memcpy(data, string, length);
    data[length] = 0;

    return data;
}
