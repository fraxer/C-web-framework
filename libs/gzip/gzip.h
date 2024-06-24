#ifndef __GZIP__
#define __GZIP__

#include <zlib.h>

struct connection;
struct http1response;

typedef struct gzip {
    int is_deflate_init; // -1 nothing, 0 deflate, 1 inflate
    z_stream stream;
    int(*deflate)(struct connection* connection, const char* data, size_t length, int end, ssize_t(*callback)(struct connection*, const char*, size_t, int));
    int(*inflate)(struct connection* connection, const char* data, size_t length);
    int(*reset)(struct gzip*);
    int(*free)(struct gzip*);
} gzip_t;

int gzip_init(gzip_t* gzip);

#endif