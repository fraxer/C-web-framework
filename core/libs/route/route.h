#ifndef __ROUTE__
#define __ROUTE__

#include <pcre.h>

#include "request.h"
#include "response.h"

typedef enum route_methods {
    ROUTE_NONE = -1,
    ROUTE_GET = 0,
    ROUTE_POST,
    ROUTE_PUT,
    ROUTE_DELETE,
    ROUTE_OPTIONS,
    ROUTE_PATCH,
    ROUTE_HEAD
} route_methods_e;

typedef struct route_param {
    unsigned short int start;
    unsigned short int end;
    size_t string_len;
    char* string;
    struct route_param* next;
} route_param_t;

typedef struct route {
    int location_erroffset;
    int is_primitive;
    int params_count;
    char* path;
    size_t path_length;
    const char* location_error;
    pcre* location;
    route_param_t* param;
    struct route* next;
    void(*handler[7])(void*);
} route_t;

route_t* route_create(const char*);
int route_set_http_handler(route_t*, const char*, void(*)(void*));
int route_set_websockets_handler(route_t*, const char*, void(*)(void*));
void routes_free(route_t* route);
int route_compare_primitive(route_t*, const char*, size_t);

#endif
