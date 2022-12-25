#include <string.h>
#include "../request/http1request.h"
#include "../response/http1response.h"
    #include <stdio.h>

void view(http1request_t* request, http1response_t* response) {
    // printf("run view handler\n");

    const char* data = "Response";

    size_t length = strlen(data);

    // printf("%ld %s\n", length, data);

    // response->header_add(response, "key", "value");
    // response->headern_add(response, "key", 3, "value", 5);

    // response->header_remove(response, "key");
    // response->headern_remove(response, "key", 3);

    response->data(response, data);

    // response->datan(response, data, length);

    // response->file(response, "/darek-zabrocki-mg-tree-town1-003-final-darekzabrocki.jpg"); // path
}

void user(http1request_t* request, http1response_t* response) {
    // printf("run user handler\n");
}
