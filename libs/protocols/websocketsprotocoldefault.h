#ifndef __WEBSOCKETSPROTOCOLDEFAULT__
#define __WEBSOCKETSPROTOCOLDEFAULT__

#include "websocketsrequest.h"

typedef struct websockets_protocol_default {
    websockets_protocol_t base;
    char*(*payload)(struct websockets_protocol_default*);
    file_content_t(*payload_file)(struct websockets_protocol_default*);
    jsondoc_t*(*payload_json)(struct websockets_protocol_default*);
} websockets_protocol_default_t;

websockets_protocol_t* websockets_protocol_default_create();

#endif
