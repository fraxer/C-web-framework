#include "http1.h"
#include "websockets.h"

void get(http1request_t* request, http1response_t* response) {
    (void)request;

    const char* data = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.";

    response->data(response, data);
}

void websocket(http1request_t* request, http1response_t* response) {
    switch_to_websockets(request, response);
}
