#ifndef __SMTPREQUEST__
#define __SMTPREQUEST__

#include "connection.h"
#include "file.h"
#include "request.h"

#define SMTPREQUEST_COMMAND_SIZE 1024

typedef struct smtprequest {
    request_t base;
    char command[SMTPREQUEST_COMMAND_SIZE];

    connection_t* connection;
} smtprequest_t;

typedef struct smtprequest_data {
    request_t base;
    char* content;
    size_t content_size;

    connection_t* connection;
} smtprequest_data_t;

smtprequest_t* smtprequest_create(connection_t*);
smtprequest_data_t* smtprequest_data_create(connection_t*);

#endif
