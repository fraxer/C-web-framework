#ifndef __WEBSOCKETSREQUEST__
#define __WEBSOCKETSREQUEST__

#include "../route/route.h"
#include "../connection/connection.h"
#include "../protocols/websocketscommon.h"
#include "request.h"

typedef struct websocketsrequest {
    request_t base;

    int frame_opcode;

    size_t uri_length;
    size_t path_length;
    size_t ext_length;
    size_t payload_length;

    const char* uri;
    const char* path;
    const char* ext;

    char* payload;

    int* keepalive_enabled;

    websockets_frame_t frame;

    websockets_query_t* query;
    websockets_query_t* last_query;

    connection_t* connection;
} websocketsrequest_t;

websocketsrequest_t* websocketsrequest_create(connection_t*);

#endif
