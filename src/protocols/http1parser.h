#ifndef __HTTP1PARSER__
#define __HTTP1PARSER__

#include "../connection/connection.h"
#include "../request/http1request.h"

typedef enum http1_request_stage {
    SPACE = 0,
    METHOD,
    URI,
    PROTOCOL,
    HEADER_KEY,
    HEADER_VALUE,
    PAYLOAD
} http1_request_stage_e;

typedef struct http1parser {
    http1_request_stage_e stage;
    int carriage_return;
    size_t bytes_readed;
    size_t pos_start;
    size_t pos;
    size_t string_len;
    char* string;
    char* buffer;
    connection_t* connection;
} http1parser_t;

void http1parser_init(http1parser_t*, connection_t*, char*);

void http1parser_free(http1parser_t*);

int http1parser_run(http1parser_t*);

int http1parser_set_bytes_readed(http1parser_t*, int);

int http1parser_set_uri(http1request_t*, const char*, size_t);

void http1parser_append_query(http1request_t*, http1_query_t*);

#endif