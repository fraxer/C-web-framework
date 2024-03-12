#ifndef __SMTPRESPONSE__
#define __SMTPRESPONSE__

#include "connection.h"
#include "response.h"

#define SMTPRESPONSE_MESSAGE_SIZE 1024

typedef struct smtpresponse {
    response_t base;

    int status;
    char message[SMTPRESPONSE_MESSAGE_SIZE];
    void* parser;

    connection_t* connection;
} smtpresponse_t;

smtpresponse_t* smtpresponse_create(connection_t*);

#endif
