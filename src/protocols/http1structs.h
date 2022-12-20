#ifndef __HTTP1HEADERS__
#define __HTTP1HEADERS__

typedef struct http1_header {
    const char* key;
    const char* value;
    struct http1_header* next;
} http1_header_t;

typedef enum http1_version {
    HTTP1_VER_NONE = 0,
    HTTP1_VER_1_0,
    HTTP1_VER_1_1
} http1_version_e;

typedef struct http1_query {
    const char* key;
    const char* value;
    struct http1_query* next;
} http1_query_t;

typedef struct http1_file {
    int fd;
    size_t size;
    size_t path_length;
    const char* path;
} http1_file_t;

#endif
