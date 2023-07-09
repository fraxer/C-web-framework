#ifndef __WEBSOCKETSREQUEST__
#define __WEBSOCKETSREQUEST__

#include "route.h"
#include "connection.h"
#include "websocketscommon.h"
#include "request.h"

typedef enum websockets_datatype {
    WEBSOCKETS_NONE = 0,
    WEBSOCKETS_CONTINUE = 0x80,
    WEBSOCKETS_TEXT = 0x81,
    WEBSOCKETS_BINARY = 0x82,
    WEBSOCKETS_CLOSE = 0x88,
    WEBSOCKETS_PING = 0x89,
    WEBSOCKETS_PONG = 0x8A
} websockets_datatype_e;

typedef struct websocketsrequest {
    request_t base;
    route_methods_e method;
    websockets_datatype_e type;
    websockets_datatype_e control_type;

    size_t uri_length;
    size_t path_length;
    size_t ext_length;
    size_t payload_length;
    size_t control_payload_length;

    char* uri;
    char* path;
    char* ext;

    char* payload;
    char* control_payload;

    int* keepalive_enabled;

    websockets_query_t* query_;
    websockets_query_t* last_query;

    connection_t* connection;

    db_t*(*database_list)(struct websocketsrequest*);
    const char*(*query)(struct websocketsrequest*, const char*);
} websocketsrequest_t;

websocketsrequest_t* websocketsrequest_create(connection_t*);

void websocketsrequest_reset(websocketsrequest_t*);

void websocketsrequest_reset_continue(websocketsrequest_t*);

#endif
