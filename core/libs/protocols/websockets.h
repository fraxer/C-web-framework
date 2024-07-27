#ifndef __WEBSOCKETS__
#define __WEBSOCKETS__

#include "websocketsrequest.h"
#include "websocketsresponse.h"
#include "websocketsswitch.h"
#include "websocketsprotocoldefault.h"
#include "websocketsprotocolresource.h"

void websockets_default_handler(websocketsrequest_t*, websocketsresponse_t*);

#endif