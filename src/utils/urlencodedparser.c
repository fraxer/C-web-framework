#include "urlencodedparser.h"

int urlencodedparser_add_part(urlencodedparser_t*);
int urlencodedparser_set_field(urlencodedparser_t*);

void urlencodedparser_init(urlencodedparser_t* parser, int payload_fd, size_t payload_size) {
    parser->payload_size = payload_size;
    parser->payload_offset = 0;
    parser->offset = 0;
    parser->size = 0;
    parser->part = NULL;
    parser->last_part = NULL;
    parser->find_amp = 1;
    parser->part_count = 0;
    parser->payload_fd = payload_fd;
}

void urlencodedparser_parse(urlencodedparser_t* parser, char* buffer, size_t buffer_size) {
    for (size_t i = 0; i < buffer_size; i++) {
        switch (buffer[i]) {
        case '=':
        {
            if (!parser->find_amp) {
                parser->size++;
                break;
            }
            parser->find_amp = 0;

            if (urlencodedparser_add_part(parser) == -1) return;
            if (urlencodedparser_set_field(parser) == -1) return;

            break;
        }
        case '&':
        {
            parser->find_amp = 1;

            if (urlencodedparser_add_part(parser) == -1) return;

            break;
        }
        default:
            parser->size++;
        }

        parser->payload_offset++;
    }

    if (parser->payload_offset == parser->payload_size) {
        if (urlencodedparser_add_part(parser) == -1) return;

        if (parser->find_amp) {
            if (urlencodedparser_set_field(parser) == -1) return;
            if (urlencodedparser_add_part(parser) == -1) return;
        }
    }
}

int urlencodedparser_add_part(urlencodedparser_t* parser) {
    http1_payloadpart_t* part = http1_payloadpart_create();
    if (!part) return -1;

    if (!parser->part) {
        parser->part = part;
    }
    if (parser->last_part) {
        parser->last_part->next = part;
    }
    parser->last_part = part;

    part->offset = parser->offset;
    part->size = parser->size;

    parser->offset = parser->payload_offset + 1;
    parser->size = 0;
    parser->part_count++;

    return 0;
}

int urlencodedparser_set_field(urlencodedparser_t* parser) {
    http1_payloadpart_t* part = parser->last_part;
    part->field = http1_payloadfield_create();
    part->field->value = malloc(part->size + 1);
    if (!part->field->value) return -1;

    off_t payload_offset = lseek(parser->payload_fd, 0, SEEK_CUR);

    lseek(parser->payload_fd, part->offset, SEEK_SET);

    ssize_t r = read(parser->payload_fd, part->field->value, part->size);
    if (r < 0) return -1;

    part->field->value[part->size] = 0;

    lseek(parser->payload_fd, payload_offset, SEEK_SET);

    return 0;
}

http1_payloadpart_t* urlencodedparser_part(urlencodedparser_t* parser) {
    return parser->part;
}