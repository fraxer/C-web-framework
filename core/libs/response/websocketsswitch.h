#ifndef __WEBSOCKETSSWITCH__
#define __WEBSOCKETSSWITCH__

#include <string.h>

#include "protocolmanager.h"
#include "http1response.h"
#include "http1request.h"
#include "sha1.h"
#include "base64.h"

void switch_to_websockets(http1request_t*, http1response_t*);

#endif