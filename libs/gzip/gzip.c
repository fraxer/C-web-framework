#include <string.h>

#include "http1response.h"
#include "connection.h"
#include "config.h"
#include "gzip.h"

static int __gzip_reset(gzip_t* gzip);
static int __gzip_free(gzip_t* gzip);
static int __gzip_deflate(connection_t* connection, const char* data, size_t length, int end, ssize_t(*callback)(connection_t*, const char*, size_t, int));
static int __gzip_inflate(connection_t* connection, const char* data, size_t length);

int gzip_init(gzip_t* gzip) {
    memset(&gzip->stream, 0, sizeof(z_stream));
    gzip->is_deflate_init = -1;
    gzip->deflate = __gzip_deflate;
    gzip->inflate = __gzip_inflate;
    gzip->reset = __gzip_reset;
    gzip->free = __gzip_free;

    return 1;
}

int __gzip_reset(gzip_t* gzip) {
    if (gzip->is_deflate_init == 0)
        return deflateReset(&gzip->stream) == Z_OK;
    else if (gzip->is_deflate_init == 1)
        return inflateReset(&gzip->stream) == Z_OK;

    return 0;
}

int __gzip_free(gzip_t* gzip) {
    if (gzip->is_deflate_init == 0)
        return deflateEnd(&gzip->stream) == Z_OK;
    else if (gzip->is_deflate_init == 1)
        return inflateEnd(&gzip->stream) == Z_OK;

    return 0;
}

int __gzip_deflate(connection_t* connection, const char* data, size_t length, int end, ssize_t(*callback)(connection_t*, const char*, size_t, int)) {
    const size_t buffer_length = config()->main.read_buffer;
    unsigned char buffer[buffer_length];
    int result = 0;
    z_stream* stream = &connection->gzip.stream;

    if (connection->gzip.is_deflate_init == -1) {
        connection->gzip.is_deflate_init = 0;

        stream->zalloc = Z_NULL;
        stream->zfree = Z_NULL;
        stream->opaque = Z_NULL;

        if (deflateInit2(stream, Z_BEST_SPEED, Z_DEFLATED, MAX_WBITS + 16, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY) != Z_OK)
            goto failed;
    }

    stream->avail_in = (uInt)length;
    stream->next_in = (Bytef*)data;

    const int flush = end ? Z_FINISH : Z_SYNC_FLUSH;
    do {
        stream->avail_out = buffer_length;
        stream->next_out = buffer;

        int ret = deflate(stream, flush);
        switch (ret) {
            case Z_NEED_DICT:
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
            case Z_BUF_ERROR:
            case Z_STREAM_ERROR:
            {
                connection->gzip.is_deflate_init = -1;
                (void)deflateEnd(stream);
                goto failed;
            }
        }

        size_t writed = buffer_length - stream->avail_out;
        if (writed > 0 && !callback(connection, (const char*)buffer, writed, end))
            goto failed;
    } while (stream->avail_out == 0);

    result = 1;

    failed:

    if (end || !result) {
        connection->gzip.is_deflate_init = -1;
        (void)deflateEnd(stream);
    }

    return result;
}

int __gzip_inflate(connection_t* connection, const char* data, size_t length) {
    http1response_t* response = (http1response_t*)connection->response;
    const size_t max_buffer_size = config()->main.read_buffer;
    const size_t buffer_length = length < max_buffer_size ? max_buffer_size : length;
    unsigned char buffer[buffer_length];
    int result = 0;
    int ret = 0;
    z_stream* stream = &connection->gzip.stream;

    if (connection->gzip.is_deflate_init == -1) {
        connection->gzip.is_deflate_init = 1;

        stream->zalloc = Z_NULL;
        stream->zfree = Z_NULL;
        stream->opaque = Z_NULL;
        stream->avail_in = 0;
        stream->next_in = Z_NULL;

        if (inflateInit2(stream, MAX_WBITS + 16) != Z_OK)
            goto failed;
    }

    stream->avail_in = (uInt)(length);
    stream->next_in = (Bytef*)data;

    do {
        stream->avail_out = buffer_length;
        stream->next_out = buffer;

        ret = inflate(stream, Z_NO_FLUSH);
        switch (ret) {
            case Z_NEED_DICT:
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
            case Z_STREAM_ERROR:
            {
                connection->gzip.is_deflate_init = -1;
                (void)inflateEnd(stream);
                goto failed;
            }
        }

        size_t writed = buffer_length - stream->avail_out;
        if (writed > 0 && !response->payload_.file.append_content(&response->payload_.file, (const char*)buffer, writed))
            goto failed;
    } while (stream->avail_out == 0);

    result = 1;

    failed:

    if (ret == Z_STREAM_END) {
        connection->gzip.is_deflate_init = -1;
        (void)inflateEnd(stream);
    }

    return result;
}


