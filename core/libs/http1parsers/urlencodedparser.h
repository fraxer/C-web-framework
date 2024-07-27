#ifndef __HTTP1URLENCODEDPARSER__
#define __HTTP1URLENCODEDPARSER__

#include <stdlib.h>
#include <stddef.h>

#include "http1common.h"

typedef struct urlencodedparser {
    size_t payload_size;
    size_t payload_offset;
    size_t offset;
    size_t size;
    http1_payloadpart_t* part;
    http1_payloadpart_t* last_part;
    int find_amp;
    int part_count;
    int payload_fd;
} urlencodedparser_t;

void urlencodedparser_init(urlencodedparser_t*, int, size_t);

void urlencodedparser_parse(urlencodedparser_t*, char*, size_t);

http1_payloadpart_t* urlencodedparser_part(urlencodedparser_t*);

#endif