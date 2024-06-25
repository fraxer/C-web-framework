#include "http1teparser.h"
#include "helpers.h"

static const long __hextable[] = { 
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1, 0,1,2,3,4,5,6,7,8,9,-1,-1,-1,-1,-1,-1,-1,10,11,12,13,14,15,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
};

int __http1teparser_set_chunk_size(http1teparser_t*);
ssize_t __http1teparser_hex_to_dec(const char*, size_t);
int __http1teparser_read_chunk(http1teparser_t*);

http1teparser_t* http1teparser_init() {
    http1teparser_t* parser = malloc(sizeof * parser);
    if (parser == NULL) return NULL;

    parser->stage = HTTP1TEPARSER_CHUNKSIZE;
    parser->bytes_readed = 0;
    parser->pos_start = 0;
    parser->pos = 0;
    parser->chunk_size = 0;
    parser->chunk_size_readed = 0;
    parser->buffer = NULL;
    parser->connection = NULL;

    bufferdata_init(&parser->buf);

    return parser;
}

void http1teparser_free(http1teparser_t* parser) {
    if (parser->buf.dynamic_buffer) free(parser->buf.dynamic_buffer);
    parser->buf.dynamic_buffer = NULL;

    free(parser);
}

void http1teparser_set_buffer(http1teparser_t* parser, char* buffer, size_t buffer_size) {
    parser->buffer = buffer;
    parser->bytes_readed = buffer_size;
}

int http1teparser_run(http1teparser_t* parser) {
    parser->pos_start = 0;
    parser->pos = 0;

    for (parser->pos = parser->pos_start; parser->pos < parser->bytes_readed; parser->pos++) {
        char ch = parser->buffer[parser->pos];

        switch (parser->stage)
        {
        case HTTP1TEPARSER_CHUNKSIZE:
            if (ch == '\r') {
                if (bufferdata_writed(&parser->buf) > 8)
                    return HTTP1TEPARSER_ERROR;

                parser->stage = HTTP1TEPARSER_CHUNKSIZE_NEWLINE;

                bufferdata_complete(&parser->buf);
                if (!__http1teparser_set_chunk_size(parser))
                    return HTTP1TEPARSER_ERROR;

                if (parser->chunk_size == 0)
                    return HTTP1TEPARSER_COMPLETE;

                bufferdata_reset(&parser->buf);
            }
            else {
                if (bufferdata_writed(&parser->buf) > 8)
                    return HTTP1TEPARSER_ERROR;

                bufferdata_push(&parser->buf, ch);
            }
            break;
        case HTTP1TEPARSER_CHUNKSIZE_NEWLINE:
            if (ch == '\n') {
                parser->stage = HTTP1TEPARSER_CHUNK;
                break;
            }
            else
                return HTTP1TEPARSER_ERROR;
        case HTTP1TEPARSER_CHUNK:
            {
                if (!__http1teparser_read_chunk(parser))
                    return HTTP1TEPARSER_ERROR;

                if (parser->chunk_size_readed >= parser->chunk_size) {
                    ch = parser->buffer[parser->pos];
                    if (ch == '\r')
                        parser->stage = HTTP1TEPARSER_CHUNK_NEWLINE;
                }

                break;
            }
            break;
        case HTTP1TEPARSER_CHUNK_NEWLINE:
            if (ch == '\n') {
                parser->stage = HTTP1TEPARSER_CHUNKSIZE;
                break;
            }
            else
                return HTTP1TEPARSER_ERROR;
        }
    }

    return HTTP1TEPARSER_CONTINUE;
}

int __http1teparser_set_chunk_size(http1teparser_t* parser) {
    char* string = bufferdata_get(&parser->buf);
    size_t length = bufferdata_writed(&parser->buf);

    ssize_t result = __http1teparser_hex_to_dec(string, length);
    if (result == -1)
        return 0;

    parser->chunk_size = (size_t)result;
    parser->chunk_size_readed = 0;

    return 1;
}

ssize_t __http1teparser_hex_to_dec(const char* data, size_t size) {
    ssize_t ret = 0;
    for (size_t pos = 0; pos < size && ret >= 0; pos++) {
        unsigned char hex = data[pos];
        ret = (ret << 4) | __hextable[hex];
        if (ret > INT_MAX) return -1;
    }
    return ret; 
}

int __http1teparser_read_chunk(http1teparser_t* parser) {
    size_t temp_chunk_size = parser->chunk_size - parser->chunk_size_readed > parser->bytes_readed - parser->pos
        ? parser->bytes_readed - parser->pos
        : parser->chunk_size - parser->chunk_size_readed;

    http1response_t* response = (http1response_t*)parser->connection->response;

    if (response->content_encoding == CE_GZIP) {
        gzip_t* const gzip = &parser->connection->gzip;
        char buffer[GZIP_BUFFER];

        if (!gzip_inflate_init(gzip, &parser->buffer[parser->pos], temp_chunk_size))
            return 0;

        do {
            const size_t writed = gzip_inflate(gzip, buffer, GZIP_BUFFER);
            if (gzip_inflate_has_error(gzip))
                return 0;

            if (writed > 0)
                response->payload_.file.append_content(&response->payload_.file, buffer, writed);
        } while (gzip_want_continue(gzip));

        if (gzip_is_end(gzip))
            if (!gzip_inflate_reset(gzip))
                return 0;
    }
    else {
        if (!response->payload_.file.append_content(&response->payload_.file, &parser->buffer[parser->pos], temp_chunk_size))
            return 0;
    }

    parser->chunk_size_readed += temp_chunk_size;
    parser->pos += temp_chunk_size;

    return 1;
}
