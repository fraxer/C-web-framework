#ifndef __PROTOCOLMANAGER__
#define __PROTOCOLMANAGER__

#include "tls.h"
#include "http1.h"
#include "websockets.h"

void protmgr_set_tls(connection_t*);

void protmgr_set_http1(connection_t*);

void protmgr_set_websockets(connection_t*);

#endif
