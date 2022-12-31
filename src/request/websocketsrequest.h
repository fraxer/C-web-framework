#ifndef __WEBSOCKETSREQUEST__
#define __WEBSOCKETSREQUEST__

#include "../route/route.h"
#include "../connection/connection.h"
#include "../protocols/websocketscommon.h"
#include "request.h"

typedef enum websockets_datatype {
    WEBSOCKETS_NONE = 0,
    WEBSOCKETS_TEXT = 0x81,
    WEBSOCKETS_BINARY = 0x82
} websockets_datatype_e;

typedef struct websocketsrequest {
    request_t base;

    websockets_datatype_e type;

    size_t uri_length;
    size_t path_length;
    size_t ext_length;
    size_t payload_length;

    const char* uri;
    const char* path;
    const char* ext;

    char* payload;

    int* keepalive_enabled;

    websockets_query_t* query;
    websockets_query_t* last_query;

    connection_t* connection;
} websocketsrequest_t;

websocketsrequest_t* websocketsrequest_create(connection_t*);

int websocketsrequest_save_payload(websocketsrequest_t*, const char*, size_t);

#endif
