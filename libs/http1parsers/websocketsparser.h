#ifndef __WEBSOCKETSPARSER__
#define __WEBSOCKETSPARSER__

#include <unistd.h>

#include "websocketsrequest.h"
#include "websocketscommon.h"

#define WSPARSER_BUFSIZ 1024

enum websocketsparser_status {
    WSPARSER_ERROR = 0,
    WSPARSER_CONTINUE,
    WSPARSER_OUT_OF_MEMORY,
    WSPARSER_BAD_REQUEST,
    WSPARSER_HOST_NOT_FOUND,
    WSPARSER_PAYLOAD_LARGE
};

enum websocketsparser_opcode {
    WSOPCODE_CONTINUE = 0x00,
    WSOPCODE_TEXT = 0x01,
    WSOPCODE_BINARY = 0x02,
    WSOPCODE_CLOSE = 0x08,
    WSOPCODE_PING = 0x09,
    WSOPCODE_PONG = 0x0A,
};

typedef enum websockets_request_stage {
    WSPARSER_FIRST_BYTE = 0,
    WSPARSER_SECOND_BYTE,
    WSPARSER_PAYLOAD_LEN_126,
    WSPARSER_PAYLOAD_LEN_127,
    WSPARSER_MASK_KEY,
    WSPARSER_PAYLOAD,
    WSPARSER_CONTROL_PAYLOAD,
    WSPARSER_COMPLETE
} websockets_request_stage_e;

typedef enum websockets_payload_stage {
    WSPAYLOAD_METHOD,
    WSPAYLOAD_LOCATION,
    WSPAYLOAD_DATA
} websockets_payload_stage_e;

typedef enum websocketsparser_buffer_type {
    STATIC = 0,
    DYNAMIC,
} websocketsparser_buffer_type_e;

typedef struct websocketsparser_buffer {
    char static_buffer[WSPARSER_BUFSIZ];
    char* dynamic_buffer;
    size_t offset_sbuffer;
    size_t offset_dbuffer;
    size_t dbuffer_size;
    websocketsparser_buffer_type_e type;
} websocketsparser_buffer_t;

typedef struct websocketsparser {
    websockets_request_stage_e stage;
    websockets_frame_t frame;
    websocketsparser_buffer_t buf;

    int mask_index;

    size_t bytes_readed;
    size_t pos_start;
    size_t pos;
    ssize_t payload_index;
    size_t payload_saved_length;

    char* buffer;
    connection_t* connection;
} websocketsparser_t;

void websocketsparser_init(websocketsparser_t*);

void websocketsparser_set_connection(websocketsparser_t*, connection_t*);

void websocketsparser_set_buffer(websocketsparser_t*, char*);

void websocketsparser_reset(websocketsparser_t*);

void websocketsparser_free(websocketsparser_t*);

void websocketsparser_set_bytes_readed(websocketsparser_t*, size_t);

int websocketsparser_run(websocketsparser_t*);

int websocketsparser_buffer_push(websocketsparser_t*, char);

void websocketsparser_buffer_reset(websocketsparser_t*);

size_t websocketsparser_buffer_writed(websocketsparser_t*);

int websocketsparser_buffer_complete(websocketsparser_t*);

int websocketsparser_buffer_move(websocketsparser_t*);

char* websocketsparser_buffer_get(websocketsparser_t*);

char* websocketsparser_buffer_copy(websocketsparser_t*);

#endif
