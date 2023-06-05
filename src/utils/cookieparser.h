#ifndef __HTTP1COOKIEPARSER__
#define __HTTP1COOKIEPARSER__

#include <stdlib.h>
#include <stddef.h>
#include "../protocols/http1common.h"

typedef struct cookieparser {
    size_t payload_offset;
    size_t offset;
    size_t size;
    http1_cookie_t* cookie;
    http1_cookie_t* last_cookie;
} cookieparser_t;

void cookieparser_init(cookieparser_t*);

void cookieparser_parse(cookieparser_t*, char*, size_t);

http1_cookie_t* cookieparser_cookie(cookieparser_t*);

#endif