#ifndef __HTTP1PARSER__
#define __HTTP1PARSER__

#include "../connection/connection.h"
#include "../request/http1request.h"

#define HTTP1PARSER_BUFSIZ 1024

enum http1parser_status {
    HTTP1PARSER_ERROR = 0,
    HTTP1PARSER_SUCCESS,
    HTTP1PARSER_CONTINUE,
    HTTP1PARSER_OUT_OF_MEMORY,
    HTTP1PARSER_BAD_REQUEST,
    HTTP1PARSER_HOST_NOT_FOUND,
    HTTP1PARSER_PAYLOAD_LARGE
};

typedef enum http1_request_stage {
    METHOD = 0,
    URI,
    PROTOCOL,
    NEWLINE1,
    HEADER_KEY,
    HEADER_SPACE,
    HEADER_VALUE,
    NEWLINE2,
    NEWLINE3,
    PAYLOAD
} http1_request_stage_e;

typedef enum http1parser_buffer_type {
    STATIC = 0,
    DYNAMIC,
} http1parser_buffer_type_e;

typedef struct http1parser_buffer {
    char static_buffer[HTTP1PARSER_BUFSIZ];
    char* dynamic_buffer;
    size_t offset_sbuffer;
    size_t offset_dbuffer;
    size_t dbuffer_size;
    http1parser_buffer_type_e type;
} http1parser_buffer_t;

typedef struct http1parser {
    http1_request_stage_e stage;
    int host_found;
    size_t bytes_readed;
    size_t pos_start;
    size_t pos;
    size_t content_length;
    size_t content_saved_length;
    char* buffer;
    connection_t* connection;

    http1parser_buffer_t buf;
} http1parser_t;

void http1parser_init(http1parser_t*, connection_t*, char*);

void http1parser_free(http1parser_t*);

int http1parser_run(http1parser_t*);

void http1parser_set_bytes_readed(http1parser_t*, int);

int http1parser_set_uri(http1request_t*, const char*, size_t);

void http1parser_append_query(http1request_t*, http1_query_t*);

http1_ranges_t* http1parser_parse_range(char*, size_t);

int http1parser_buffer_push(http1parser_t*, char);

void http1parser_buffer_reset(http1parser_t*);

size_t http1parser_buffer_writed(http1parser_t*);

int http1parser_buffer_complete(http1parser_t*);

int http1parser_buffer_move(http1parser_t*);

char* http1parser_buffer_get(http1parser_t*);

char* http1parser_buffer_copy(http1parser_t*);

int http1parser_payload_loaded(http1parser_t*);

#endif