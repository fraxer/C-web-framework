#ifndef __HTTP1MULTIPARTPARSER__
#define __HTTP1MULTIPARTPARSER__

#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "http1common.h"

typedef enum multipartstage {
    BODY = 0,
    BOUNDARY_FN,
    FIRST_DASH,
    SECOND_DASH,
    BOUNDARY,
    BOUNDARY_SR,
    BOUNDARY_SN,
    BOUNDARY_FD,
    BOUNDARY_SD,
    HEADER_KEY,
    HEADER_SPACE,
    HEADER_VALUE,
    HEADER_N,
    END_N,
} multipartstage_e;

typedef struct multipartparser {
    const char* boundary;
    size_t boundary_size;
    size_t payload_size;
    size_t payload_offset;
    size_t boundary_offset;
    size_t header_key_offset;
    size_t header_value_offset;
    size_t header_key_size;
    size_t header_value_size;
    size_t offset;
    size_t size;
    multipartstage_e stage;
    http1_payloadpart_t* part;
    http1_payloadpart_t* last_part;
    http1_header_t* header;
    http1_header_t* last_header;
    int payload_fd;
} multipartparser_t;

void multipartparser_init(multipartparser_t*, int, const char*);

void multipartparser_parse(multipartparser_t*, char*, size_t);

http1_payloadpart_t* multipartparser_part(multipartparser_t*);

#endif