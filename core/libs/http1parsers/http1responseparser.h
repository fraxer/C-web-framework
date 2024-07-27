#ifndef __HTTP1RESPONSEPARSER__
#define __HTTP1RESPONSEPARSER__

#include "connection.h"
#include "http1parsercommon.h"
#include "http1teparser.h"
#include "http1response.h"
#include "bufferdata.h"

typedef enum http1responseparser_stage {
    HTTP1RESPONSEPARSER_PROTOCOL = 0,
    HTTP1RESPONSEPARSER_STATUS_CODE,
    HTTP1RESPONSEPARSER_STATUS_TEXT,
    HTTP1RESPONSEPARSER_NEWLINE1,
    HTTP1RESPONSEPARSER_HEADER_KEY,
    HTTP1RESPONSEPARSER_HEADER_SPACE,
    HTTP1RESPONSEPARSER_HEADER_VALUE,
    HTTP1RESPONSEPARSER_NEWLINE2,
    HTTP1RESPONSEPARSER_NEWLINE3,
    HTTP1RESPONSEPARSER_PAYLOAD,
    HTTP1RESPONSEPARSER_COMPLETE
} http1responseparser_stage_e;

typedef struct http1responseparser {
    http1responseparser_stage_e stage;
    bufferdata_t buf;
    size_t bytes_readed;
    size_t pos_start;
    size_t pos;
    size_t content_length;
    size_t content_saved_length;
    char* buffer;
    connection_t* connection;
    http1teparser_t* teparser;
} http1responseparser_t;

void http1responseparser_init(http1responseparser_t*);

void http1responseparser_set_connection(http1responseparser_t*, connection_t*);

void http1responseparser_set_buffer(http1responseparser_t*, char*);

void http1responseparser_free(http1responseparser_t*);

void http1responseparser_reset(http1responseparser_t*);

int http1responseparser_run(http1responseparser_t*);

void http1responseparser_set_bytes_readed(http1responseparser_t*, int);

int http1responseparser_set_uri(http1response_t*, const char*, size_t);

void http1responseparser_append_query(http1response_t*, http1_query_t*);

http1_ranges_t* http1responseparser_parse_range(char*, size_t);

#endif