#include "websockets.h"

void websockets_default_handler(websocketsrequest_t* request, websocketsresponse_t* response) {
    if (request->type == WEBSOCKETS_TEXT) {
        response->text(response, "");
        return;
    }

    response->binary(response, "");
}