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

struct websocketsrequest;

typedef struct websockets_protocol {
    int(*payload_parse)(struct websocketsrequest*, const char*, size_t, int);
    int(*get_resource)(connection_t*);
    void(*reset)(void*);
    void(*free)(void*);
} websockets_protocol_t;

typedef struct websocketsrequest {
    request_t base;
    websockets_datatype_e type;
    websockets_datatype_e control_type;
    websockets_payload_t payload;
    void* parser;
    websockets_protocol_t* protocol;

    size_t payload_length;
    int fragmented;

    int* keepalive_enabled;

    connection_t* connection;

    db_t*(*database_list)(struct websocketsrequest*);
} websocketsrequest_t;

websocketsrequest_t* websocketsrequest_create(connection_t*);

void websocketsrequest_reset(websocketsrequest_t*);

void websocketsrequest_reset_continue(websocketsrequest_t*);

#endif
