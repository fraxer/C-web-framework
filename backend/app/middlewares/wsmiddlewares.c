#include <string.h>
#include <arpa/inet.h>

#include "websockets.h"
#include "wsmiddlewares.h"
#include "ratelimiter.h"
#include "log.h"

int middleware_ws_query_param_required(wsctx_t* ctx, char** keys, int size) {
    websockets_protocol_resource_t* protocol = (websockets_protocol_resource_t*)ctx->request->protocol;
    char message[256] = {0};
    for (int i = 0; i < size; i++) {
        const char* param = protocol->get_query(protocol, keys[i]);
        if (param == NULL || param[0] == 0) {
            sprintf(message, "param <%s> not found", keys[i]);
            ctx->response->send_data(ctx, message);
            return 0;
        }
    }

    return 1;
}
