#ifndef __ROUTE__
#define __ROUTE__

#include "pcre.h"

typedef enum route_methods {
    ROUTE_NONE = -1,
    ROUTE_GET = 0,
    ROUTE_POST = 1,
    ROUTE_PUT = 2,
    ROUTE_DELETE = 3,
    ROUTE_OPTIONS = 4,
    ROUTE_PATCH = 5
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
    void*(*method[6])(void*);
} route_t;

void* route_init();

route_t* route_create(const char*);

int route_set_method_handler(route_t* route, const char* method, void* function);

void route_free(route_t*);

int route_compare_primitive(route_t*, const char*, size_t);

#endif
