#ifndef __HTTP1TEPARSER__
#define __HTTP1TEPARSER__

#include "connection.h"
#include "http1parsercommon.h"
#include "http1response.h"
#include "bufferdata.h"

typedef enum http1teparser_status {
    HTTP1TEPARSER_ERROR = 0,
    HTTP1TEPARSER_CONTINUE,
    HTTP1TEPARSER_COMPLETE
} http1teparser_status_e;

typedef enum http1teparser_stage {
    HTTP1TEPARSER_CHUNKSIZE = 0,
    HTTP1TEPARSER_CHUNKSIZE_NEWLINE,
    HTTP1TEPARSER_CHUNK,
    HTTP1TEPARSER_CHUNK_NEWLINE
} http1teparser_stage_e;

typedef struct http1teparser {
    http1teparser_stage_e stage;
    bufferdata_t buf;
    size_t bytes_readed;
    size_t pos_start;
    size_t pos;
    size_t chunk_size;
    size_t chunk_size_readed;
    char* buffer;
    connection_t* connection;
} http1teparser_t;

http1teparser_t* http1teparser_init();
void http1teparser_free(http1teparser_t*);
void http1teparser_set_buffer(http1teparser_t*, char*, size_t);
int http1teparser_run(http1teparser_t*);

#endif