#ifndef __HTTP1COMMON__
#define __HTTP1COMMON__

typedef struct http1_header {
    const char* key;
    const char* value;
    size_t key_length;
    size_t value_length;
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

typedef struct http1_body {
    size_t size;
    size_t pos;
    char* data;
} http1_body_t;

typedef struct http1_file {
    size_t size;
    size_t pos;
    int fd;
} http1_file_t;

http1_header_t* http1_header_create(const char*, size_t, const char*, size_t);

http1_query_t* http1_query_create();

const char* http1_query_set_field(const char*, size_t);

#endif
