#ifndef __WSMIDDLEWARES__
#define __WSMIDDLEWARES__

#include "wsctx.h"
#include "middleware.h"

/**
 * Middleware that validates presence of required query parameters in WebSocket handshake.
 * Sends error response if any parameter is missing or empty.
 * @param ctx   WebSocket context
 * @param keys  Array of required parameter names
 * @param size  Number of parameters in array
 * @return 1 if all parameters present, 0 if any missing (stops chain)
 */
int middleware_ws_query_param_required(wsctx_t* ctx, char** keys, int size);

#endif
