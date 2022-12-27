#ifndef __WEBSOCKETSCOMMON__
#define __WEBSOCKETSCOMMON__

typedef struct websockets_query {
    const char* key;
    const char* value;
    struct websockets_query* next;
} websockets_query_t;

typedef struct websockets_body {
    size_t size;
    size_t pos;
    char* data;
} websockets_body_t;

typedef struct websockets_file {
    size_t size;
    size_t pos;
    int fd;
} websockets_file_t;

websockets_query_t* websockets_query_create(const char*, size_t, const char*, size_t);

void websockets_query_free(websockets_query_t*);

const char* websockets_set_field(const char*, size_t);

#endif
