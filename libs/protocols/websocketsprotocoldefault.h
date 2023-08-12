#ifndef __WEBSOCKETSPROTOCOLDEFAULT__
#define __WEBSOCKETSPROTOCOLDEFAULT__

#include "websocketsrequest.h"

typedef struct websockets_protocol_default {
    websockets_protocol_t base;
    char*(*payload)(websocketsrequest_t*);
    char*(*payloadf)(websocketsrequest_t*);
    char*(*payload_json)(websocketsrequest_t*);
} websockets_protocol_default_t;

websockets_protocol_default_t* websockets_protocol_default_create();

#endif
