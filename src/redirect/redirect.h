#ifndef __REDIRECT__
#define __REDIRECT__

#include "pcre.h"

typedef struct redirect_param {
    size_t start;
    size_t end;
    struct redirect_param* next;
} redirect_param_t;

typedef struct redirect {
    int location_erroffset;
    char* template;
    size_t template_length;
    const char* location_error;
    pcre* location;
    redirect_param_t* param;
    struct redirect* next;
} redirect_t;

redirect_t* redirect_create(const char*, const char*);

int redirect_set_http_handler(redirect_t*, const char*, void*);

void redirect_free(redirect_t*);

#endif
