#ifndef __WEBSOCKETSPROTOCOLRESOURCE__
#define __WEBSOCKETSPROTOCOLRESOURCE__

#include "connection.h"
#include "route.h"
#include "websocketsrequest.h"

typedef struct websockets_protocol_resource {
    websockets_protocol_t base;
    route_methods_e method;
    size_t uri_length;
    size_t path_length;
    size_t ext_length;

    char* uri;
    char* path;
    char* ext;

    websockets_query_t* query_;

    char*(*payload)(websocketsrequest_t*);
    char*(*payloadf)(websocketsrequest_t*);
    char*(*payload_json)(websocketsrequest_t*);
    const char*(*query)(struct websockets_protocol_resource*, const char*);
} websockets_protocol_resource_t;

websockets_protocol_resource_t* websockets_protocol_resource_create();

#endif
