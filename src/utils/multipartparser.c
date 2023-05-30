#include "multipartparser.h"
#include "formdataparser.h"

int multipartparser_create_part(multipartparser_t*);
int multipartparser_create_header(multipartparser_t*);
void multipartparser_reset_header(multipartparser_t*);
int multipartparser_write_header(int, char*, size_t, size_t);

void multipartparser_init(multipartparser_t* parser, int payload_fd, const char* boundary) {
    parser->boundary = boundary;
    parser->boundary_size = strlen(boundary);
    parser->payload_size = lseek(payload_fd, 0, SEEK_END);
    parser->payload_offset = 0;
    parser->boundary_offset = 0;
    parser->header_key_offset = 0;
    parser->header_value_offset = 0;
    parser->header_key_size = 0;
    parser->header_value_size = 0;
    parser->offset = 0;
    parser->size = 0;
    parser->stage = BOUNDARY_FN;
    parser->part = NULL;
    parser->last_part = NULL;
    parser->header = NULL;
    parser->last_header = NULL;
    parser->payload_fd = payload_fd;

    lseek(payload_fd, 0, SEEK_SET);
}

void multipartparser_parse(multipartparser_t* parser, char* buffer, size_t buffer_size) {
    for (size_t i = 0; i < buffer_size; i++) {
        char ch = buffer[i];

        switch (parser->stage) {
        case BODY:
            if (ch == '\r')
                parser->stage = BOUNDARY_FN;
            break;
        case BOUNDARY_FN:
            if (ch == '\n')
                parser->stage = FIRST_DASH;
            else if (ch == '-')
                parser->stage = SECOND_DASH;
            else
                parser->stage = BODY;
            break;
        case FIRST_DASH:
            if (ch == '-')
                parser->stage = SECOND_DASH;
            else
                parser->stage = BODY;
            break;
        case SECOND_DASH:
            if (ch == '-') {
                parser->boundary_offset = parser->payload_offset + 1;
                parser->stage = BOUNDARY;
            }
            else
                parser->stage = BODY;
            break;
        case BOUNDARY:
        {
            size_t pos = parser->payload_offset - parser->boundary_offset;

            if (pos < parser->boundary_size) {
                if (parser->boundary[pos] != ch) {
                    parser->stage = BODY;
                }
                else if (pos == parser->boundary_size - 1) {
                    parser->stage = BOUNDARY_FD;

                    if (parser->offset > 0)
                        if (!multipartparser_create_part(parser)) return;
                }
            }
            else
                parser->stage = BODY;
            break;
        }
        case BOUNDARY_SR:
            if (ch == '\r')
                parser->stage = BOUNDARY_SN;
            break;
        case BOUNDARY_SN:
            parser->stage = HEADER_KEY;
            multipartparser_reset_header(parser);
            break;
        case BOUNDARY_FD:
            if (ch == '-')
                parser->stage = BOUNDARY_SD;
            else if (ch == '\r')
                parser->stage = BOUNDARY_SN;
            else
                goto end;
            break;
        case BOUNDARY_SD:
            if (ch == '-') {
                parser->stage = BODY;
                goto end;
            }
            break;
        case HEADER_KEY:
            if (ch == ':') {
                parser->stage = HEADER_SPACE;
                parser->header_value_offset = parser->payload_offset + 1;
            }
            else if (ch == '\r')
                parser->stage = END_N;
            else
                parser->header_key_size++;
            break;
        case HEADER_SPACE:
            if (ch == ' ')
                parser->header_value_offset = parser->payload_offset + 1;
            else if (ch != ' ') {
                parser->stage = HEADER_VALUE;
                parser->header_value_size++;
            }
            break;
        case HEADER_VALUE:
            if (ch == '\r')
                parser->stage = HEADER_N;
            else
                parser->header_value_size++;
            break;
        case HEADER_N:
            if (ch == '\n')
                if (!multipartparser_create_header(parser))
                    return;
            break;
        case END_N:
            if (ch == '\n') {
                parser->stage = BODY;
                parser->offset = parser->payload_offset + 1;
                parser->size = 0;
            }
            break;
        }
        
        parser->payload_offset++;
        parser->size++;
    }

    end:

    parser->size++;
}

http1_payloadpart_t* multipartparser_part(multipartparser_t* parser) {
    return parser->part;
}

int multipartparser_create_part(multipartparser_t* parser) {
    http1_header_t* header = parser->header;
    http1_payloadfield_t* field = NULL;
    http1_payloadfield_t* last_field = NULL;
    while (header) {
        if (strcmp(header->key, "Content-Disposition") == 0) {
            formdataparser_t fdparser;
            formdataparser_init(&fdparser, header->value_length);
            formdataparser_parse(&fdparser, header->value, header->value_length);

            formdatafield_t* hfield = formdataparser_fields(&fdparser);
            while (hfield) {
                http1_payloadfield_t* f = http1_payloadfield_create();

                f->key_length = hfield->key_size;
                f->key = http1_set_field(hfield->key, hfield->key_size);
                if (f->key == NULL) return 0;

                f->value_length = hfield->value_size;
                f->value = http1_set_field(&header->value[hfield->value_offset], hfield->value_size);
                if (f->value == NULL) return 0;

                hfield = hfield->next;

                if (!field)
                    field = f;
                if (last_field)
                    last_field->next = f;
                last_field = f;
            }

            formdataparser_free(&fdparser);
            break;
        }
        header = header->next;
    }

    http1_payloadpart_t* part = http1_payloadpart_create();
    if (part == NULL) return 0;

    part->offset = parser->offset;
    part->header = parser->header;
    part->field = field;
    part->size = parser->size - parser->boundary_size - 4;

    parser->header = NULL;
    parser->last_header = NULL;

    if (!parser->part)
        parser->part = part;
    if (parser->last_part)
        parser->last_part->next = part;
    parser->last_part = part;

    return 1;
}

int multipartparser_create_header(multipartparser_t* parser) {
    parser->stage = HEADER_KEY;

    http1_header_t* header = http1_header_create(NULL, parser->header_key_size, NULL, parser->header_value_size);
    if (header == NULL) return 0;

    if (!multipartparser_write_header(parser->payload_fd, header->key, parser->header_key_offset, parser->header_key_size))
        return 0;
    if (!multipartparser_write_header(parser->payload_fd, header->value, parser->header_value_offset, parser->header_value_size))
        return 0;

    if (!parser->header)
        parser->header = header;
    if (parser->last_header)
        parser->last_header->next = header;
    parser->last_header = header;

    return 1;
}

void multipartparser_reset_header(multipartparser_t* parser) {
    parser->header_key_offset = parser->payload_offset + 1;
    parser->header_key_size = 0;
    parser->header_value_offset = 0;
    parser->header_value_size = 0;
}

int multipartparser_write_header(int fd, char* value, size_t offset, size_t size) {
    off_t current_offset = lseek(fd, 0, SEEK_CUR);

    lseek(fd, offset, SEEK_SET);
    int r = read(fd, value, size);
    if (r < 0) return 0;

    lseek(fd, current_offset, SEEK_SET);

    value[size] = 0;

    return 1;
}
