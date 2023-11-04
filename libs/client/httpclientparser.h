#ifndef __HTTPCLIENTPARSER__
#define __HTTPCLIENTPARSER__

#include <stddef.h>

#include "http1common.h"
#include "bufferdata.h"

typedef enum httpclientparser_stage {
    CLIENTPARSER_PROTOCOL = 0,
    CLIENTPARSER_PROTOCOL_SLASH1,
    CLIENTPARSER_PROTOCOL_SLASH2,
    CLIENTPARSER_HOST,
    CLIENTPARSER_PORT,
    CLIENTPARSER_URI
} httpclientparser_stage_e;

enum httpclientparser_status {
    CLIENTPARSER_BAD_PROTOCOL = 0,
    CLIENTPARSER_BAD_PROTOCOL_SEPARATOR,
    CLIENTPARSER_BAD_HOST,
    CLIENTPARSER_BAD_PORT,
    CLIENTPARSER_BAD_URI,
    CLIENTPARSER_OK
};

typedef struct httpclientparser {
    httpclientparser_stage_e stage;
    bufferdata_t buf;
    size_t bytes_readed;
    size_t pos_start;
    size_t pos;
    unsigned int use_ssl : 1;
    short int port;
    char* host;
    char* uri;
    char* path;
    char* ext;
    char* buffer;
    http1_query_t* query;
    http1_query_t* last_query;
} httpclientparser_t;

void httpclientparser_init(httpclientparser_t* parser);
void httpclientparser_free(httpclientparser_t* parser);
void httpclientparser_reset(httpclientparser_t* parser);
int httpclientparser_parse(httpclientparser_t* parser, const char* url);
int httpclientparser_parse(httpclientparser_t* parser, const char* url);
int httpclientparser_move_use_ssl(httpclientparser_t* parser);
char* httpclientparser_move_host(httpclientparser_t* parser);
short int httpclientparser_move_port(httpclientparser_t* parser);
char* httpclientparser_move_uri(httpclientparser_t* parser);
char* httpclientparser_move_path(httpclientparser_t* parser);
char* httpclientparser_move_ext(httpclientparser_t* parser);
http1_query_t* httpclientparser_move_query(httpclientparser_t* parser);
http1_query_t* httpclientparser_move_last_query(httpclientparser_t* parser);

#endif