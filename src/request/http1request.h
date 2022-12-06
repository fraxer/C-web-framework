#ifndef __HTTP1REQUEST__
#define __HTTP1REQUEST__

#include "../route/route.h"
#include "request.h"

typedef enum http1request_version {
    HTTP1_VER_NONE = 0,
    HTTP1_VER_1_0,
    HTTP1_VER_1_1
} http1request_version_e;

typedef struct http1request_header {
    const char* key;
    const char* value;
    struct http1request_header* next;
} http1request_header_t;

typedef struct http1request_query {
    const char* key;
    const char* value;
    struct http1request_query* next;
} http1request_query_t;

typedef struct http1request {
    request_t base;
    route_methods_e method;
    http1request_version_e version;
    size_t uri_length;
    size_t path_length;
    const char* uri;
    const char* path;
    char* payload;
    http1request_query_t* query;
    http1request_header_t* header;
    http1request_header_t* last_header;
} http1request_t;

http1request_t* http1request_create();

#endif
