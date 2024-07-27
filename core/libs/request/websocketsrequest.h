#ifndef __WEBSOCKETSREQUEST__
#define __WEBSOCKETSREQUEST__

#include "route.h"
#include "connection.h"
#include "websocketscommon.h"
#include "json.h"
#include "request.h"
#include "file.h"

struct websocketsrequest;

typedef struct websockets_protocol {
    websockets_payload_t payload;
    int(*payload_parse)(struct websocketsrequest*, char*, size_t);
    int(*get_resource)(connection_t*);
    void(*reset)(void*);
    void(*free)(void*);
} websockets_protocol_t;

typedef struct websocketsrequest {
    request_t base;
    websockets_datatype_e type;
    void* parser;
    websockets_protocol_t* protocol;

    int can_reset;
    int fragmented;

    int* keepalive_enabled;

    connection_t* connection;
} websocketsrequest_t;

websocketsrequest_t* websocketsrequest_create(connection_t*);
void websocketsrequest_reset(websocketsrequest_t*);
void websocketsrequest_reset_continue(websocketsrequest_t*);

char* websocketsrequest_payload(websockets_protocol_t*);
file_content_t websocketsrequest_payload_file(websockets_protocol_t*);
jsondoc_t* websocketsrequest_payload_json(websockets_protocol_t*);

void websockets_protocol_init_payload(websockets_protocol_t*);
int websockets_create_tmpfile(websockets_protocol_t*, const char*);

#endif
