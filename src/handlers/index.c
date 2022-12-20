#include <string.h>
#include "../request/http1request.h"
#include "../response/http1response.h"
    #include <stdio.h>

void* view(void* request) {
    // printf("run user handler\n");
    return 0;
}

// void view(void* req, void* res) {
//     // printf("run view handler\n");

//     http1request_t* request = (http1request_t*)req;
//     http1response_t* response = (http1response_t*)res;

//     const char* data = request->path;

//     size_t length = strlen(data);

//     response->data(response, data);

//     response->datan(response, data, length);

//     response->header_add(response, "key", "value");
//     response->headern_add(response, "key", 3, "value", 5);

//     response->header_remove(response, "key");
//     response->headern_remove(response, "key", 3);

//     response->status(response, 200);

//     response->file(response, data); // path
// }

void* user(void* request) {
    // printf("run user handler\n");
    return 0;
}