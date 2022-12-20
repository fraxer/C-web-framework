#ifndef __HTTP1REQUEST__
#define __HTTP1REQUEST__

#include "../route/route.h"
#include "../protocols/http1structs.h"
#include "request.h"

typedef struct http1request {
    request_t base;
    route_methods_e method;
    http1_version_e version;
    size_t uri_length;
    size_t path_length;
    size_t ext_length;
    const char* uri;
    const char* path;
    const char* ext;
    char* payload;
    http1_query_t* query;
    http1_query_t* last_query;
    http1_header_t* header;
    http1_header_t* last_header;
} http1request_t;

http1request_t* http1request_create();

#endif
