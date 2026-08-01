#include <string.h>

#include "middleware_registry.h"
#include "httpmiddlewares.h"
#include "wsmiddlewares.h"
#include "log.h"

/**
 * Middleware functions registered in this list are called globally for the server context.
 * Functions should only accept one argument (context).
 * NOTE: These are registered in middlewares_init() via middleware_registry_register()
 */

/* ============= INITIALIZATION FUNCTION ============= */

int middlewares_init(void) {
    /* Register all application middlewares */

    /* Register middleware_http_forbidden */
    if (!middleware_registry_register("middleware_http_forbidden", (middleware_fn_p)middleware_http_forbidden)) {
        log_error("middlewares_init: failed to register middleware_http_forbidden\n");
        return 0;
    }

    /* Register middleware_http_test_header */
    if (!middleware_registry_register("middleware_http_test_header", (middleware_fn_p)middleware_http_test_header)) {
        log_error("middlewares_init: failed to register middleware_http_test_header\n");
        return 0;
    }

    /* Register middleware_h2c_upgrade — accepts HTTP/1.1 `Upgrade: h2c` requests
     * on plaintext vhosts. Register it in a server's "middlewares" list to turn
     * on h2c Upgrade support for that vhost. */
    if (!middleware_registry_register("middleware_h2c_upgrade", (middleware_fn_p)middleware_h2c_upgrade)) {
        log_error("middlewares_init: failed to register middleware_h2c_upgrade\n");
        return 0;
    }

    /* Add more middlewares here as needed:
     * if (!middleware_registry_register("middleware_cors", middleware_cors)) {
     *     log_error("middlewares_init: failed to register middleware_cors\n");
     *     return 0;
     * }
     */

    return 1;
}
