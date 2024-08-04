#ifndef __WEBSOCKETSSWITCH__
#define __WEBSOCKETSSWITCH__

#include <string.h>

#include "protocolmanager.h"
#include "http1response.h"
#include "http1request.h"
#include "httpcontext.h"
#include "sha1.h"
#include "base64.h"

void switch_to_websockets(httpctx_t* ctx);

#endif