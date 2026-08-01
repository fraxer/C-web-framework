#ifndef __HTTPMIDDLEWARES__
#define __HTTPMIDDLEWARES__

#include "httpctx.h"
#include "middleware.h"

/**
 * Middleware that blocks request with 403 Forbidden response.
 * Always stops middleware chain execution.
 * @param ctx  HTTP context
 * @return 0 (always stops chain)
 */
int middleware_http_forbidden(httpctx_t* ctx);

/**
 * Test middleware that adds X-Test-Header to response.
 * Used for debugging and testing middleware chain.
 * @param ctx  HTTP context
 * @return 1 (continues chain)
 */
int middleware_http_test_header(httpctx_t* ctx);

/**
 * Middleware that validates presence of required query parameters.
 * Sends error response if any parameter is missing or empty.
 * @param ctx   HTTP context
 * @param keys  Array of required parameter names
 * @param size  Number of parameters in array
 * @return 1 if all parameters present, 0 if any missing (stops chain)
 */
int middleware_http_query_param_required(httpctx_t* ctx, char** keys, int size);

/**
 * Authentication middleware.
 * Validates session cookie and loads user data into context.
 * Checks: session_id cookie exists, session is valid, user_id in session, user exists in database.
 * On success, user is available via ctx->user_data (cast to user_t*).
 * @param ctx  HTTP context
 * @return 1 if authenticated, 0 on auth failure (stops chain)
 */
int middleware_http_auth(httpctx_t* ctx);

/**
 * h2c upgrade middleware (RFC 9113 §3.2). On a plaintext request that carries
 * `Upgrade: h2c` + `HTTP2-Settings`, performs the HTTP/1.1 → HTTP/2 upgrade:
 * stages a 101 Switching Protocols response and arranges the protocol switch.
 * The actual session is built once the 101 is flushed. Register it on plaintext
 * vhosts to accept h2c Upgrade clients (e.g. nghttp).
 * @param ctx  HTTP context
 * @return 0 if the request was upgraded (stops the chain — the 101 stands),
 *         1 for an ordinary request (continues to the handler)
 */
int middleware_h2c_upgrade(httpctx_t* ctx);

#endif
