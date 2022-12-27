#ifndef __WEBSOCKETSRESPONSE__
#define __WEBSOCKETSRESPONSE__

#include "../connection/connection.h"
#include "../protocols/websocketscommon.h"
#include "response.h"

typedef struct websocketsresponse {
    response_t base;

    websockets_body_t body;
    websockets_file_t file_;

    connection_t* connection;

    void(*data)(struct websocketsresponse*, const char*);
    void(*datan)(struct websocketsresponse*, const char*, size_t);
    int(*file)(struct websocketsresponse*, const char*);
    int(*filen)(struct websocketsresponse*, const char*, size_t);
} websocketsresponse_t;

websocketsresponse_t* websocketsresponse_create(connection_t*);

void websocketsresponse_default_response(websocketsresponse_t*, int);

int websocketsresponse_data_append(char*, size_t*, const char*, size_t);

#endif
