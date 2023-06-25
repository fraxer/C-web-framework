#include "websockets.h"

void echo(websocketsrequest_t* request, websocketsresponse_t* response) {
    const char* data = "";

    if (request->payload)
        data = request->payload;

    response->text(response, data);
}
