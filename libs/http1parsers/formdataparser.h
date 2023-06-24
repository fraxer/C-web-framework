#ifndef __HTTP1FORMDATAPARSER__
#define __HTTP1FORMDATAPARSER__

#include <stdlib.h>
#include <stddef.h>

#define FORMDATABUFSIZ 256
#define FORMDATAKEY 1
#define FORMDATAVALUE 2

typedef struct formdatalocation {
    int ok;
    size_t offset;
    size_t size;
} formdatalocation_t;

typedef struct formdatafield {
    char* key;
    size_t key_size;
    size_t value_offset;
    size_t value_size;
    struct formdatafield* next;
} formdatafield_t;

typedef struct formdataparser {
    char buffer[FORMDATABUFSIZ];
    char* type;
    int quote;
    int cmp;
    char prev_symbol;
    size_t type_size;
    size_t payload_size;
    size_t payload_offset;
    size_t offset;
    size_t size;
    formdatafield_t* field;
    formdatafield_t* last_field;
} formdataparser_t;

void formdataparser_init(formdataparser_t*, size_t);

void formdataparser_free(formdataparser_t*);

void formdataparser_parse(formdataparser_t*, const char*, size_t);

formdatalocation_t formdataparser_field(formdataparser_t*, const char*);

formdatafield_t* formdataparser_fields(formdataparser_t*);

#endif