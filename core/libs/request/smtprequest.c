#include <string.h>
#include <unistd.h>

#include "smtprequest.h"

void __smtprequest_reset(void*);
void __smtprequest_free(void*);
void __smtprequest_data_reset(void*);
void __smtprequest_data_free(void*);

smtprequest_t* smtprequest_create(connection_t* connection) {
    smtprequest_t* request = malloc(sizeof * request);
    if (request == NULL) return NULL;

    memset(request->command, 0, SMTPREQUEST_COMMAND_SIZE);
    request->connection = connection;
    request->base.reset = (void(*)(void*))__smtprequest_reset;
    request->base.free = (void(*)(void*))__smtprequest_free;

    return request;
}

smtprequest_data_t* smtprequest_data_create(connection_t* connection) {
    smtprequest_data_t* request = malloc(sizeof * request);
    if (request == NULL) return NULL;

    request->content = NULL;
    request->content_size = 0;
    request->connection = connection;
    request->base.reset = (void(*)(void*))__smtprequest_data_reset;
    request->base.free = (void(*)(void*))__smtprequest_data_free;

    return request;
}

void __smtprequest_reset(void* arg) {
    smtprequest_t* request = (smtprequest_t*)arg;

    memset(request->command, 0, SMTPREQUEST_COMMAND_SIZE);
}

void __smtprequest_free(void* arg) {
    smtprequest_t* request = (smtprequest_t*)arg;

    __smtprequest_reset(request);

    free(request);
}

void __smtprequest_data_reset(void* arg) {
    smtprequest_data_t* request = (smtprequest_data_t*)arg;

    if (request->content != NULL) {
        free(request->content);
        request->content = NULL;
    }

    request->content_size = 0;
}

void __smtprequest_data_free(void* arg) {
    smtprequest_data_t* request = (smtprequest_data_t*)arg;

    __smtprequest_data_reset(request);

    free(request);
}
