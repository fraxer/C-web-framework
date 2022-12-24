#ifndef __HTTP1RESPONSE__
#define __HTTP1RESPONSE__

#include "../protocols/http1common.h"
#include "response.h"

typedef struct http1response {
    response_t base;

    int status_code;

    http1_body_t body;
    http1_file_t file_;
    http1_header_t* header;
    http1_header_t* last_header;

    void(*data)(struct http1response*, const char*);
    void(*datan)(struct http1response*, const char*, size_t);
    int(*header_add)(struct http1response*, const char*, const char*);
    int(*headern_add)(struct http1response*, const char*, size_t, const char*, size_t);
    int(*header_add_content_length)(struct http1response*, size_t);
    int(*header_add_content_type)(struct http1response*, const char*, size_t);
    int(*header_remove)(struct http1response*, const char*);
    int(*headern_remove)(struct http1response*, const char*, size_t);
    int(*status)(struct http1response*, int);
    int(*file)(struct http1response*, const char*);
    int(*filen)(struct http1response*, const char*, size_t);
} http1response_t;

http1response_t* http1response_create();

void http1response_reset();

#endif
