#ifndef __REDIRECT__
#define __REDIRECT__

#include <pcre.h>

enum redirect_status {
    REDIRECT_OUT_OF_MEMORY,
    REDIRECT_LOOP_CYCLE,
    REDIRECT_FOUND,
    REDIRECT_NOT_FOUND
};

typedef struct redirect_param {
    int number;
    size_t start;
    size_t end;
    struct redirect_param* next;
} redirect_param_t;

typedef struct redirect {
    int location_erroffset;
    int params_count;
    char* template;
    size_t template_length;
    const char* location_error;
    pcre* location;
    redirect_param_t* param;
    struct redirect* next;
} redirect_t;

redirect_t* redirect_create(const char*, const char*);

void redirect_free(redirect_t*);

char* redirect_get_uri(redirect_t*, const char*, int*);

#endif
