#ifndef __WEBSOCKETSPARSER__
#define __WEBSOCKETSPARSER__

#include <unistd.h>
#include "../connection/connection.h"

typedef enum websockets_request_stage {
    FIRST_BYTE = 0,
    SECOND_BYTE,
    PAYLOAD_LEN,
    MASK_KEY,
    DATA
} websockets_request_stage_e;

typedef struct websocketsparser {
    websockets_request_stage_e stage;
    unsigned int fin;
    unsigned int rsv1;
    unsigned int rsv2;
    unsigned int rsv3;
    unsigned int opcode;
    unsigned int masked;
    ssize_t payload_length;
    unsigned int mask;

    size_t bytes_readed;
    size_t pos_start;
    size_t pos;
    size_t string_len;

    unsigned char* string;
    char* buffer;
    connection_t* connection;
} websocketsparser_t;

void websocketsparser_init(websocketsparser_t*, connection_t*, char*);

void websocketsparser_set_bytes_readed(websocketsparser_t*, size_t);

int websocketsparser_run(websocketsparser_t*);

#endif
