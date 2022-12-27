#include <string.h>
#include "../connection/connection.h"
#include "websocketsparser.h"

int websocketsparser_parse_first_byte(websocketsparser_t*);
int websocketsparser_parse_second_byte(websocketsparser_t*);
int websocketsparser_parse_payload_length(websocketsparser_t*);
int websocketsparser_parse_mask(websocketsparser_t*);
int websocketsparser_parse_payload(websocketsparser_t*);
int websocketsparser_string_append(websocketsparser_t*);
int websocketsparser_set_payload_length(websocketsparser_t*, const unsigned char*, size_t);


void websocketsparser_init(websocketsparser_t* parser, connection_t* connection, char* buffer) {
    parser->stage = FIRST_BYTE;

    parser->fin = 0;
    parser->rsv1 = 0;
    parser->rsv2 = 0;
    parser->rsv3 = 0;
    parser->opcode = 0;
    parser->masked = 0;
    parser->payload_length = 0;
    parser->mask = 0;

    parser->bytes_readed = 0;
    parser->pos_start = 0;
    parser->pos = 0;
    parser->string_len = 0;
    parser->string = NULL;
    parser->buffer = (unsigned char*)buffer;
    parser->connection = connection;
}

void websocketsparser_set_bytes_readed(websocketsparser_t* parser, size_t bytes_readed) {
	parser->bytes_readed = bytes_readed;
}

int websocketsparser_run(websocketsparser_t* parser) {
    int result = 0;

    parser->pos_start = 0;
    parser->pos = 0;

    switch (parser->stage) {
    case FIRST_BYTE:
        result = websocketsparser_parse_first_byte(parser);
        if (result < 0) return result;
    case SECOND_BYTE:
        result = websocketsparser_parse_second_byte(parser);
        if (result < 0) return result;
    case PAYLOAD_LEN:
        result = websocketsparser_parse_payload_length(parser);
        if (result < 0) return result;
    case MASK_KEY:
        result = websocketsparser_parse_mask(parser);
        if (result < 0) return result;
    case DATA:
        result = websocketsparser_parse_payload(parser);
        if (result < 0) return result;
    }

    return result;

    // unsigned char fin    = (buffer[0] >> 7) & 0x01;
    // unsigned char opcode =  buffer[0] & 0x0F;
    // unsigned char masked = (buffer[1] >> 7) & 0x01;

    // unsigned long long payload_length = 0;
    // int pos = 2;
    // int length_field = buffer[1] & (~0x80);
    // unsigned int mask = 0;

    // if (length_field <= 125) {
    //     payload_length = length_field;
    // }
    // else if (length_field == 126) {
    //     payload_length = (
    //         (buffer[2] << 8) | 
    //         (buffer[3])
    //     );
    //     pos += 2;
    // }
    // else if (length_field == 127) {
    //     int byteCount = 8;

    //     while (--byteCount > 1) {
    //         payload_length |= (buffer[byteCount + 1] & 0xFF) << (8 * byteCount);
    //     }

    //     payload_length |= buffer[9];

    //     // payload_length = (
    //     //     (buffer[2] << 56) | 
    //     //     (buffer[3] << 48) | 
    //     //     (buffer[4] << 40) | 
    //     //     (buffer[5] << 32) | 
    //     //     (buffer[6] << 24) | 
    //     //     (buffer[7] << 16) | 
    //     //     (buffer[8] << 8) | 
    //     //     (buffer[9])
    //     // );

    //     pos += 8;
    // }

    // printf("payload_length: %lld\n", payload_length);

    // if (masked) {
    //     mask = *((unsigned int*)(buffer + pos));
    //     pos += 4;

    //     unsigned char* c = buffer + pos;

    //     for (unsigned long long i = 0; i < payload_length; i++) {
    //         c[i] = c[i] ^ ((unsigned char*)(&mask))[i % 4];
    //     }
    // }

    // char* out_buffer = (char*)malloc(payload_length + 1);

    // memcpy(out_buffer, buffer + pos, payload_length);

    // out_buffer[payload_length] = 0;

    // unsigned short num = ((unsigned char)(out_buffer[0]) | (unsigned char)(out_buffer[1]) << 8);

    // // printf("%d\n", num);
    // // printf("%s\n", out_buffer);
    // // printf("%d, %d\n", (unsigned char)(out_buffer[0]), (unsigned char)(out_buffer[1]));

    // printf("num %d\n", num);
    // printf("Fin %d\n", fin);
    // printf("Opcode %d\n", opcode);
    // printf("Masked %d\n", masked);
    // printf("%s\n", out_buffer);

	return 0;
}

int websocketsparser_parse_first_byte(websocketsparser_t* parser) {
    unsigned char c = parser->buffer[0];

    parser->fin = (c >> 7) & 0x01;
    parser->rsv1 = (c >> 6) & 0x01;
    parser->rsv2 = (c >> 5) & 0x01;
    parser->rsv3 = (c >> 4) & 0x01;
    parser->opcode = c & 0x0F;

    printf("fin: %d\n", parser->fin);
    printf("rsv1: %d\n", parser->rsv1);
    printf("rsv2: %d\n", parser->rsv2);
    printf("rsv3: %d\n", parser->rsv3);
    printf("opcode: %d\n", parser->opcode);

    parser->stage = SECOND_BYTE;

    parser->pos_start++;

    if (parser->pos + 1 == parser->bytes_readed) return -2;

    return 0;
}

int websocketsparser_parse_second_byte(websocketsparser_t* parser) {
    if (parser->pos_start > parser->bytes_readed) return -1;

    unsigned char c = parser->buffer[parser->pos_start];

    parser->masked = (c >> 7) & 0x01;
    parser->payload_length = c & (~0x80);

    printf("masked: %d\n", parser->masked);
    printf("payload_length: %ld\n", parser->payload_length);

    if (parser->payload_length <= 125) {
        parser->stage = MASK_KEY;
    }
    else {
        parser->stage = PAYLOAD_LEN;
    }

    parser->pos_start = parser->pos + 1;

    if (parser->pos + 1 == parser->bytes_readed) return -2;

    return 0;
}

int websocketsparser_parse_payload_length(websocketsparser_t* parser) {
    for (parser->pos = parser->pos_start; parser->pos < parser->bytes_readed; parser->pos++) {
        int payload_string_length = parser->string_len + (parser->pos - parser->pos_start);

        if (parser->payload_length == 126 && payload_string_length == 2) goto next;
        if (parser->payload_length == 127 && payload_string_length == 8) goto next;
    }

    return websocketsparser_string_append(parser);

    next:

    if (parser->string && websocketsparser_string_append(parser) == -1) return -1;

    if (parser->string) {
        if (websocketsparser_set_payload_length(parser, parser->string, parser->string_len) == -1) return -1;
    } else {
        if (websocketsparser_set_payload_length(parser, &parser->buffer[parser->pos_start], parser->pos - parser->pos_start) == -1) return -1;
    }

    parser->pos_start = parser->pos + 1;

    if (parser->string) {
        free(parser->string);
        parser->string = NULL;
    }

    parser->string_len = 0;

    parser->stage = MASK_KEY;

    if (parser->pos + 1 == parser->bytes_readed) return -2;

    return 0;
}

int websocketsparser_parse_mask(websocketsparser_t*) {
    return 0;
}

int websocketsparser_parse_payload(websocketsparser_t*) {
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

    // printf("%d .%s.\n", parser->string_len, parser->string);

    return -2;
}

int websocketsparser_set_payload_length(websocketsparser_t* parser, const unsigned char* string, size_t length) {
    ssize_t payload_length = 0;
    int byte_count = 0;

    if (parser->payload_length == 126) {
        // payload_length = (
        //     (string[0] << 8) | 
        //     (string[1])
        // );

        byte_count = 2;
    }
    else if (parser->payload_length == 127) {
        // payload_length = (
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

    parser->payload_length = 0;

    while (--byte_count >= 0) {
        parser->payload_length |= ((unsigned char)string[byte_count] & 0xFF) << (8 * (8 - byte_count + 1));
    }

    printf("real payload_length: %ld\n", parser->payload_length);

    return 0;
}