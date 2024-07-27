#include <string.h>
#include <unistd.h>

#include "smtpresponse.h"
#include "smtpresponseparser.h"

void __smtpresponse_reset(void*);
void __smtpresponse_free(void*);
int __smtpresponse_init_parser(smtpresponse_t*);

smtpresponse_t* smtpresponse_create(connection_t* connection) {
    smtpresponse_t* response = malloc(sizeof * response);
    if (response == NULL) return NULL;

    response->status = 0;
    response->connection = connection;
    memset(response->message, 0, SMTPRESPONSE_MESSAGE_SIZE);
    response->base.reset = __smtpresponse_reset;
    response->base.free = __smtpresponse_free;

    if (!__smtpresponse_init_parser(response)) {
        free(response);
        return NULL;
    }

    return response;
}

void __smtpresponse_reset(void* arg) {
    smtpresponse_t* response = (smtpresponse_t*)arg;

    response->status = 0;
    memset(response->message, 0, SMTPRESPONSE_MESSAGE_SIZE);

    smtpresponseparser_reset(response->parser);
}

void __smtpresponse_free(void* arg) {
    smtpresponse_t* response = (smtpresponse_t*)arg;

    __smtpresponse_reset(response);
    smtpresponseparser_free(response->parser);

    free(response);
}

int __smtpresponse_init_parser(smtpresponse_t* response) {
    response->parser = malloc(sizeof(smtpresponseparser_t));
    if (response->parser == NULL) return 0;

    smtpresponseparser_init(response->parser);

    return 1;
}