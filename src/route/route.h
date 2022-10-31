#ifndef __ROUTE__
#define __ROUTE__

#include "pcre.h"

#define ROUTE_EMPTY_PATH "Route error: Empty path\n"
#define ROUTE_OUT_OF_MEMORY "Route error: Out of memory\n"
#define ROUTE_EMPTY_TOKEN "Route error: Empty token in \"%s\"\n"
#define ROUTE_UNOPENED_TOKEN "Route error: Unopened token in \"%s\"\n"
#define ROUTE_UNCLOSED_TOKEN "Route error: Unclosed token in \"%s\"\n"
#define ROUTE_EMPTY_PARAM_NAME "Route error: Empty param name in \"%s\"\n"
#define ROUTE_EMPTY_PARAM_EXPRESSION "Route error: Empty param expression in \"%s\"\n"
#define ROUTE_PARAM_ONE_WORD "Route error: For param need one word in \"%s\"\n"

enum route_methods {
    ROUTE_NONE = -1,
    ROUTE_GET = 0,
    ROUTE_POST = 1,
    ROUTE_PUT = 2,
    ROUTE_DELETE = 3,
    ROUTE_OPTIONS = 4,
    ROUTE_PATCH = 5
};

typedef struct route_param {
    unsigned short int start;
    unsigned short int end;
    size_t string_len;
    char* string;
    struct route_param* next;
} route_param_t;

typedef struct route {
    char* path;
    const char* dest;
    const char* location_error;
    const char* rule_error;

    void*(*method[6])(void*);

    int location_erroffset;
    int rule_erroffset;

    pcre* location;

    route_param_t* param;

    struct route* next;
} route_t;

void* route_init();

route_t* route_create(const char*, const char*);

route_t* route_get_first_route();

int route_set_method_handler(route_t* route, const char* method, void* function);

void route_free();

#endif
