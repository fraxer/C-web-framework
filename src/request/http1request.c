#include <stddef.h>
#include <stdlib.h>
#include "http1request.h"

http1request_t* http1request_alloc() {
    return (http1request_t*)malloc(sizeof(http1request_t));
}

void http1request_header_free(http1request_header_t* header) {

}

void http1request_free(void* arg) {
    http1request_t* request = (http1request_t*)arg;

    request->method = HTTP1_NONE;

    if (request->uri) free(request->uri);
    request->uri = NULL;

    if (request->path) free(request->path);
    request->path = NULL;

    if (request->query) free(request->query);
    request->query = NULL;

    if (request->payload) free(request->payload);
    request->payload = NULL;

    http1request_header_free(request->header);
    request->header = NULL;

    free(request);

    request = NULL;
}

http1request_t* http1request_create() {
    http1request_t* request = http1request_alloc();

    if (request == NULL) return NULL;

    request->method = HTTP1_NONE;
    request->version = HTTP1_VER_NONE;
    request->uri = NULL;
    request->path = NULL;
    request->query = NULL;
    request->payload = NULL;
    request->header = NULL;
    request->last_header = NULL;
    request->base.free = (void(*)(void*))http1request_free;

    return request;
}