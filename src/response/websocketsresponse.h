#ifndef __WEBSOCKETSRESPONSE__
#define __WEBSOCKETSRESPONSE__

#include "../connection/connection.h"
#include "../protocols/websocketscommon.h"
#include "response.h"

typedef struct websocketsresponse {
    response_t base;

    unsigned char frame_code;

    websockets_body_t body;
    websockets_file_t file_;

    connection_t* connection;

    void(*text)(struct websocketsresponse*, const char*);
    void(*textn)(struct websocketsresponse*, const char*, size_t);
    void(*binary)(struct websocketsresponse*, const char*);
    void(*binaryn)(struct websocketsresponse*, const char*, size_t);
    int(*file)(struct websocketsresponse*, const char*);
    int(*filen)(struct websocketsresponse*, const char*, size_t);
} websocketsresponse_t;

websocketsresponse_t* websocketsresponse_create(connection_t*);

void websocketsresponse_default_response(websocketsresponse_t*, const char*);

int websocketsresponse_data_append(char*, size_t*, const char*, size_t);

void websocketsresponse_pong(websocketsresponse_t*, const char*, size_t);

void websocketsresponse_close(websocketsresponse_t*, const char*, size_t);

#endif
