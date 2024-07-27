#ifndef __DOMAIN__
#define __DOMAIN__

#include "pcre.h"

typedef struct domain {
    int pcre_erroffset;
    char* template;
    char* prepared_template;
    const char* pcre_error;
    pcre* pcre_template;
    struct domain* next;
} domain_t;

domain_t* domain_create(const char*);

domain_t* domain_alloc(const char*);

void domains_free(domain_t*);

int domain_parse(domain_t*);

int domain_count(domain_t*);

#endif
