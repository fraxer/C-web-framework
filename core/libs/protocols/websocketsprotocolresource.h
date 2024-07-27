#ifndef __WEBSOCKETSPROTOCOLRESOURCE__
#define __WEBSOCKETSPROTOCOLRESOURCE__

#include "connection.h"
#include "route.h"
#include "websocketsrequest.h"

typedef enum {
    WSPROTRESOURCE_METHOD = 0,
    WSPROTRESOURCE_LOCATION,
    WSPROTRESOURCE_DATA
} websockets_protocol_resource_stage_e;

typedef struct websockets_protocol_resource {
    websockets_protocol_t base;
    route_methods_e method;
    websockets_protocol_resource_stage_e parser_stage;
    size_t uri_length;
    size_t path_length;
    size_t ext_length;

    char* uri;
    char* path;
    char* ext;

    websockets_query_t* query_;

    char*(*payload)(struct websockets_protocol_resource*);
    file_content_t(*payload_file)(struct websockets_protocol_resource*);
    jsondoc_t*(*payload_json)(struct websockets_protocol_resource*);
    const char*(*query)(struct websockets_protocol_resource*, const char*);
} websockets_protocol_resource_t;

websockets_protocol_t* websockets_protocol_resource_create();

#endif
