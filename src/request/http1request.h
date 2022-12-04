#ifndef __HTTP1REQUEST__
#define __HTTP1REQUEST__

#include "request.h"

typedef enum http1request_version {
    HTTP1_VER_NONE = 0,
    HTTP1_VER_1_0,
    HTTP1_VER_1_1
} http1request_version_e;

typedef enum http1request_method {
    HTTP1_NONE = 0,
    HTTP1_GET,
    HTTP1_PUT,
    HTTP1_POST,
    HTTP1_PATCH,
    HTTP1_DELETE,
    HTTP1_OPTIONS
} http1request_method_e;

typedef struct http1request_header {
    char* key;
    char* value;
    struct http1request_header* next;
} http1request_header_t;

typedef struct http1request {
    request_t base;
    http1request_method_e method;
    http1request_version_e version;
    char* uri;
    char* path;
    char* query;
    char* payload;
    http1request_header_t* header;
    http1request_header_t* last_header;
} http1request_t;

http1request_t* http1request_create();

#endif
