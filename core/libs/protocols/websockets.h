#ifndef __WEBSOCKETS__
#define __WEBSOCKETS__

#include "websocketsrequest.h"
#include "websocketsresponse.h"
#include "websocketsswitch.h"
#include "websocketsprotocoldefault.h"
#include "websocketsprotocolresource.h"
#include "wscontext.h"

void websockets_default_handler(wsctx_t* ctx);

#endif