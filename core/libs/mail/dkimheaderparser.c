#include <stdlib.h>
#include <ctype.h>

#include "dkimheaderparser.h"

dkimheaderparser_t* dkimheaderparser_alloc() {
    return malloc(sizeof(dkimheaderparser_t));
}

void dkimheaderparser_init(dkimheaderparser_t* parser) {
    parser->stage = DKIMHEADERPARSER_SPACE;
    parser->buffer_size = 0;
    parser->pos_start = 0;
    parser->pos = 0;
    parser->buffer = NULL;

    bufferdata_init(&parser->buf);
}

void dkimheaderparser_set_buffer(dkimheaderparser_t* parser, const char* buffer, const size_t buffer_size) {
    parser->buffer = buffer;
    parser->buffer_size = buffer_size;
}

void dkimheaderparser_flush(dkimheaderparser_t* parser) {
    if (parser->buf.dynamic_buffer) free(parser->buf.dynamic_buffer);
    parser->buf.dynamic_buffer = NULL;
}

void dkimheaderparser_free(dkimheaderparser_t* parser) {
    dkimheaderparser_flush(parser);
    free(parser);
}

int dkimheaderparser_run(dkimheaderparser_t* parser) {
    bufferdata_reset(&parser->buf);
    parser->pos_start = 0;
    parser->pos = 0;

    for (parser->pos = parser->pos_start; parser->pos < parser->buffer_size; parser->pos++) {
        char ch = parser->buffer[parser->pos];

        if (isspace(ch)) {
            if (parser->stage != DKIMHEADERPARSER_SPACE)
                bufferdata_push(&parser->buf, ' ');

            parser->stage = DKIMHEADERPARSER_SPACE;
        }
        else {
            parser->stage = DKIMHEADERPARSER_SYMBOL;
            bufferdata_push(&parser->buf, ch);
        }
    }

    if (bufferdata_back(&parser->buf) == ' ')
        bufferdata_pop_back(&parser->buf);

    bufferdata_complete(&parser->buf);

    return 1;
}

char* dkimheaderparser_get_content(dkimheaderparser_t* parser) {
    return bufferdata_copy(&parser->buf);
}

size_t dkimheaderparser_get_content_length(dkimheaderparser_t* parser) {
    return bufferdata_writed(&parser->buf);
}
