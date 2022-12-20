#ifndef __HTTP1RESPONSE__
#define __HTTP1RESPONSE__

#include "../protocols/http1structs.h"
#include "response.h"

typedef struct http1response {
    response_t base;

    int status_code;

    size_t response_pos;
    size_t response_length;

    const char* body;

    http1_file_t file_;
    http1_header_t* header;
    http1_header_t* last_header;

    int(*data)(struct http1response*, const char*);
    int(*datan)(struct http1response*, const char*, size_t);
    int(*header_add)(struct http1response*, const char*, const char*);
    int(*headern_add)(struct http1response*, const char*, size_t, const char*, size_t);
    int(*header_remove)(struct http1response*, const char*);
    int(*headern_remove)(struct http1response*, const char*, size_t);
    int(*status)(struct http1response*, int);
    int(*file)(struct http1response*, const char*);
} http1response_t;

// http1response_t* http1response_create();

#endif
