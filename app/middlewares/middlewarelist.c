#include <string.h>

#include "middlewarelist.h"
#include "httpmiddlewares.h"
#include "wsmiddlewares.h"

/**
 * Middleware functions registered in this list are called globally for the server context.
 * Functions should only accept one argument (context).
 */
static middleware_global_fn_t __middleware_list[] = {
    {"middleware_http_forbidden", (middleware_fn_p)middleware_http_forbidden},
    {"middleware_http_test_header", (middleware_fn_p)middleware_http_test_header},
};

middleware_fn_p middleware_by_name(const char* name) {
    const size_t size = sizeof(__middleware_list) / sizeof(middleware_global_fn_t);

    for (size_t i = 0; i < size; i++)
        if (strcmp(__middleware_list[i].name, name) == 0)
            return __middleware_list[i].fn;

    return NULL;
}
