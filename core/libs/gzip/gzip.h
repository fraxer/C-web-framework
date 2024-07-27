#ifndef __GZIP__
#define __GZIP__

#include <zlib.h>

#define GZIP_BUFFER 16384

typedef struct gzip {
    int is_deflate_init; // -1 nothing, 0 deflate, 1 inflate
    int status_code;
    z_stream stream;
} gzip_t;

int gzip_init(gzip_t* gzip);
int gzip_free(gzip_t* gzip);

int gzip_deflate_init(gzip_t* gzip, const char* data, const size_t length);
size_t gzip_deflate(gzip_t* gzip, const char* compress_data, const size_t compress_length, const int end);
int gzip_deflate_free(gzip_t* gzip);
int gzip_deflate_has_error(gzip_t* gzip);

int gzip_inflate_init(gzip_t* gzip, const char* compress_data, const size_t compress_length);
size_t gzip_inflate(gzip_t* gzip, const char* data, const size_t length);
int gzip_inflate_free(gzip_t* gzip);
int gzip_inflate_has_error(gzip_t* gzip);

int gzip_is_end(gzip_t* gzip);
int gzip_want_continue(gzip_t* gzip);

#endif