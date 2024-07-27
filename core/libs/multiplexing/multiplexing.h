#ifndef __MULTIPLEXING__
#define __MULTIPLEXING__

#include <stdatomic.h>

#include "appconfig.h"
#include "connection.h"

enum mpxevents {
    MPXIN = 0x001,
    MPXOUT = 0x002,
    MPXERR = 0x004,
    MPXHUP = 0x008,
    MPXRDHUP = 0x010,
    MPXONESHOT = 0x020
};

typedef enum mpxtype {
    MULTIPLEX_TYPE_EPOLL = 0
} mpxtype_e;

typedef struct mpxlistener {
    connection_t* connection;
    struct mpxlistener* next;
} mpxlistener_t;

typedef struct mpxapi {
    atomic_int connection_count;
    void* config;
    mpxlistener_t* listeners;
    void(*free)(void*);
    int(*control_add)(connection_t*, int);
    int(*control_mod)(connection_t*, int);
    int(*control_del)(connection_t*);
    int(*process_events)(appconfig_t* appconfig, void* arg);
} mpxapi_t;

mpxapi_t* mpx_create(mpxtype_e);
void mpx_free(mpxapi_t*);
void mpxlistener_free(mpxlistener_t*);

#endif