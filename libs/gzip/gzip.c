#include <string.h>

#include "log.h"
#include "gzip.h"

int gzip_init(gzip_t* gzip) {
    memset(&gzip->stream, 0, sizeof(z_stream));
    gzip->is_deflate_init = -1;
    gzip->status_code = Z_OK;

    return 1;
}

int gzip_free(gzip_t* gzip) {
    if (gzip->is_deflate_init == 0)
        return gzip_deflate_free(gzip);
    else if (gzip->is_deflate_init == 1)
        return gzip_inflate_free(gzip);

    return 0;
}

int gzip_deflate_init(gzip_t* const gzip, const char* data, const size_t length) {
    z_stream* const stream = &gzip->stream;
    if (gzip->is_deflate_init == -1) {
        gzip->is_deflate_init = 0;

        stream->zalloc = Z_NULL;
        stream->zfree = Z_NULL;
        stream->opaque = Z_NULL;

        if (deflateInit2(stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, MAX_WBITS + 16, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY) != Z_OK)
            return 0;
    }

    stream->avail_in = (uInt)length;
    stream->next_in = (Bytef*)data;

    return 1;
}

size_t gzip_deflate(gzip_t* gzip, const char* compress_data, const size_t compress_length, const int end) {
    z_stream* const stream = &gzip->stream;
    stream->avail_out = (uInt)compress_length;
    stream->next_out = (Bytef*)compress_data;

    gzip->status_code = deflate(&gzip->stream, end ? Z_FINISH : Z_SYNC_FLUSH);

    return compress_length - gzip->stream.avail_out;
}

int gzip_deflate_free(gzip_t* gzip) {
    gzip->is_deflate_init = -1;

    switch (deflateEnd(&gzip->stream)) {
    case Z_OK:
    case Z_DATA_ERROR:
        return 1;
    default:
        log_error("gzip_deflate_free: Error gzip deflate free\n");
        return 0;
    }

    return 1;
}

int gzip_deflate_has_error(gzip_t* gzip) {
    return gzip->status_code < 0;
}

int gzip_inflate_init(gzip_t* gzip, const char* compress_data, const size_t compress_length) {
    z_stream* stream = &gzip->stream;
    if (gzip->is_deflate_init == -1) {
        gzip->is_deflate_init = 1;

        stream->zalloc = Z_NULL;
        stream->zfree = Z_NULL;
        stream->opaque = Z_NULL;
        stream->avail_in = 0;
        stream->next_in = Z_NULL;

        if (inflateInit2(stream, MAX_WBITS + 16) != Z_OK)
            return 0;
    }

    stream->avail_in = (uInt)compress_length;
    stream->next_in = (Bytef*)compress_data;

    return 1;
}

size_t gzip_inflate(gzip_t* gzip, const char* data, const size_t length) {
    z_stream* stream = &gzip->stream;
    stream->avail_out = (uInt)length;
    stream->next_out = (Bytef*)data;

    gzip->status_code = inflate(&gzip->stream, Z_NO_FLUSH);

    return length - gzip->stream.avail_out;
}

int gzip_inflate_free(gzip_t* gzip) {
    gzip->is_deflate_init = -1;

    if (inflateEnd(&gzip->stream) != Z_OK) {
        log_error("gzip_inflate_free: Error gzip inflate free\n");
        return 0;
    }

    return 1;
}

int gzip_inflate_has_error(gzip_t* gzip) {
    switch (gzip->status_code) {
    case Z_NEED_DICT:
    case Z_DATA_ERROR:
    case Z_MEM_ERROR:
    case Z_STREAM_ERROR:
        return 1;
    }

    return 0;
}

int gzip_is_end(gzip_t* gzip) {
    return gzip->status_code == Z_STREAM_END;
}

int gzip_want_continue(gzip_t* gzip) {
    return gzip->stream.avail_out == 0;
}
