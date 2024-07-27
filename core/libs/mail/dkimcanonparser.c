#include <stdlib.h>

#include "dkimcanonparser.h"

dkimcanonparser_t* dkimcanonparser_alloc() {
    return malloc(sizeof(dkimcanonparser_t));
}

void dkimcanonparser_init(dkimcanonparser_t* parser) {
    parser->stage = DKIMCANONPARSER_SYMBOL;
    parser->buffer_size = 0;
    parser->pos_start = 0;
    parser->pos = 0;
    parser->buffer = NULL;

    bufferdata_init(&parser->buf);
}

void dkimcanonparser_set_buffer(dkimcanonparser_t* parser, const char* buffer, const size_t buffer_size) {
    parser->buffer = buffer;
    parser->buffer_size = buffer_size;
}

void dkimcanonparser_flush(dkimcanonparser_t* parser) {
    if (parser->buf.dynamic_buffer) free(parser->buf.dynamic_buffer);
    parser->buf.dynamic_buffer = NULL;
}

void dkimcanonparser_free(dkimcanonparser_t* parser) {
    dkimcanonparser_flush(parser);
    free(parser);
}

int dkimcanonparser_run(dkimcanonparser_t* parser) {
    parser->pos_start = 0;
    parser->pos = 0;

    for (parser->pos = parser->pos_start; parser->pos < parser->buffer_size; parser->pos++) {
        char ch = parser->buffer[parser->pos];

        switch (ch)
        {
        case '\t':
        case ' ':
        {
            if (parser->stage != DKIMCANONPARSER_SPACE)
                bufferdata_push(&parser->buf, ' ');

            parser->stage = DKIMCANONPARSER_SPACE;

            break;
        }
        case '\n':
        {
            parser->stage = DKIMCANONPARSER_SYMBOL;

            bufferdata_complete(&parser->buf);

            while (1) {
                char c = bufferdata_back(&parser->buf);

                if (c == ' ' || c == '\t')
                    bufferdata_pop_back(&parser->buf);
                else
                    break;
            }

            bufferdata_push(&parser->buf, '\r');
            bufferdata_push(&parser->buf, '\n');

            break;
        }
        default:
            parser->stage = DKIMCANONPARSER_SYMBOL;
            bufferdata_push(&parser->buf, ch);
        }
    }

    while (1) {
        char c = bufferdata_back(&parser->buf);

        if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
            bufferdata_pop_back(&parser->buf);
        else
            break;
    }

    bufferdata_push(&parser->buf, '\r');
    bufferdata_push(&parser->buf, '\n');
    bufferdata_complete(&parser->buf);

    return 1;
}

char* dkimcanonparser_get_content(dkimcanonparser_t* parser) {
    return bufferdata_copy(&parser->buf);
}
