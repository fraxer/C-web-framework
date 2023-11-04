#ifndef __HTTP1REQUESTPARSER__
#define __HTTP1REQUESTPARSER__

#include "connection.h"
#include "http1parsercommon.h"
#include "http1request.h"
#include "bufferdata.h"

typedef enum http1requestparser_stage {
    HTTP1PARSER_METHOD = 0,
    HTTP1PARSER_URI,
    HTTP1PARSER_PROTOCOL,
    HTTP1PARSER_NEWLINE1,
    HTTP1PARSER_HEADER_KEY,
    HTTP1PARSER_HEADER_SPACE,
    HTTP1PARSER_HEADER_VALUE,
    HTTP1PARSER_NEWLINE2,
    HTTP1PARSER_NEWLINE3,
    HTTP1PARSER_PAYLOAD,
    HTTP1PARSER_COMPLETE
} http1requestparser_stage_e;

typedef struct http1requestparser {
    http1requestparser_stage_e stage;
    bufferdata_t buf;
    int host_found;
    size_t bytes_readed;
    size_t pos_start;
    size_t pos;
    size_t content_length;
    size_t content_saved_length;
    char* buffer;
    connection_t* connection;
} http1requestparser_t;

void http1parser_init(http1requestparser_t*);

void http1parser_set_connection(http1requestparser_t*, connection_t*);

void http1parser_set_buffer(http1requestparser_t*, char*);

void http1parser_free(http1requestparser_t*);

void http1parser_reset(http1requestparser_t*);

int http1parser_run(http1requestparser_t*);

void http1parser_set_bytes_readed(http1requestparser_t*, int);

int http1parser_set_uri(http1request_t*, const char*, size_t);

void http1parser_append_query(http1request_t*, http1_query_t*);

http1_ranges_t* http1parser_parse_range(char*, size_t);

#endif