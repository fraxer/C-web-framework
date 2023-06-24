#ifndef __WEBSOCKETSCOMMON__
#define __WEBSOCKETSCOMMON__

#include <stddef.h>
#include <sys/types.h>

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

typedef struct websockets_frame {
    unsigned int fin;
    unsigned int rsv1;
    unsigned int rsv2;
    unsigned int rsv3;
    unsigned int opcode;
    unsigned int masked;
    unsigned char mask[4];
    ssize_t payload_length;
} websockets_frame_t;

websockets_query_t* websockets_query_create(const char*, size_t, const char*, size_t);

void websockets_query_free(websockets_query_t*);

const char* websockets_set_field(const char*, size_t);

void websockets_frame_init(websockets_frame_t*);

#endif
