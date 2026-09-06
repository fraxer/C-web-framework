#include <stdio.h>

#include "model.h"
#include "httpcontext.h"
#include "wscontext.h"
#include "middleware_registry.h"
#include "httpmiddlewares.h"
#include "wsmiddlewares.h"

/**
 * Entry point of the application module.
 *
 * cwfr (and migrate) dlopen this library from `main.modules` in config.json and
 * call app_init() once, before the `servers` section is parsed -- so everything
 * the config refers to by name has to be registered here.
 *
 * On a hard reload the middleware registry is cleared and app_init() runs again;
 * it must therefore stay idempotent. The library itself is never dlclose'd.
 *
 * Errors go to stderr, not log_error(): app_init() runs before the parsed
 * configuration is published, so the logger is still silent at this point.
 *
 * @return 1 on success, 0 to abort startup
 */
int app_init(void) {
    /* Both contexts carry a user_t* in ctx->user_data (httpctx_set_user), and
     * user_t is an ORM model -- so model_free is what releases it. The core has
     * no way to know that, which is exactly why it asks.
     *
     * Checked, because ctx->user_data has exactly one owner: with several
     * modules in main.modules, a second one claiming it is a configuration
     * mistake worth failing on rather than a race to register last. */
    if (!httpctx_set_user_data_free(model_free) || !wsctx_set_user_data_free(model_free)) {
        fprintf(stderr, "app_init: context user_data destructor already claimed\n");
        return 0;
    }

    if (!middleware_registry_register("middleware_http_forbidden", (middleware_fn_p)middleware_http_forbidden)) {
        fprintf(stderr, "app_init: failed to register middleware_http_forbidden\n");
        return 0;
    }

    if (!middleware_registry_register("middleware_http_test_header", (middleware_fn_p)middleware_http_test_header)) {
        fprintf(stderr, "app_init: failed to register middleware_http_test_header\n");
        return 0;
    }

    /* middleware_h2c_upgrade — accepts HTTP/1.1 `Upgrade: h2c` requests on
     * plaintext vhosts. Register it in a server's "middlewares" list to turn on
     * h2c Upgrade support for that vhost. */
    if (!middleware_registry_register("middleware_h2c_upgrade", (middleware_fn_p)middleware_h2c_upgrade)) {
        fprintf(stderr, "app_init: failed to register middleware_h2c_upgrade\n");
        return 0;
    }

    return 1;
}
