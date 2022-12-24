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

typedef struct http1_parser {
    http1_request_stage_e stage;
    int carriage_return;
    size_t bytes_readed;
    size_t pos_start;
    size_t pos;
    size_t string_len;
    char* string;
    char* buffer;
    connection_t* connection;
} http1_parser_t;

void http1_parser_init(http1_parser_t*, connection_t*, char*);
int http1_parser_run(http1_parser_t*);
int http1_parser_set_bytes_readed(http1_parser_t*, int);
void http1_parser_append_query(http1request_t*, http1_query_t*);

#endif