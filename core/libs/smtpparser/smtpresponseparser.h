#ifndef __SMTPRESPONSEPARSER__
#define __SMTPRESPONSEPARSER__

#include "connection.h"
#include "smtpresponse.h"
#include "bufferdata.h"

typedef enum smtpresponseparser_status {
    SMTPRESPONSEPARSER_ERROR = 0,
    SMTPRESPONSEPARSER_CONTINUE,
    SMTPRESPONSEPARSER_COMPLETE
} smtpresponseparser_status_e;

typedef enum smtpresponseparser_stage {
    SMTPRESPONSEPARSER_STATUS = 0,
    SMTPRESPONSEPARSER_MESSAGE,
    SMTPRESPONSEPARSER_CONTINUE_MESSAGE,
} smtpresponseparser_stage_e;

typedef struct smtpresponseparser {
    smtpresponseparser_stage_e stage;
    bufferdata_t buf;
    size_t bytes_readed;
    size_t pos_start;
    size_t pos;
    size_t line_size;
    char* buffer;
    connection_t* connection;
} smtpresponseparser_t;

void smtpresponseparser_init(smtpresponseparser_t*);
void smtpresponseparser_set_connection(smtpresponseparser_t*, connection_t*);
void smtpresponseparser_set_buffer(smtpresponseparser_t*, char*);
void smtpresponseparser_free(smtpresponseparser_t*);
void smtpresponseparser_reset(smtpresponseparser_t*);
int smtpresponseparser_run(smtpresponseparser_t*);
void smtpresponseparser_set_bytes_readed(smtpresponseparser_t*, int);

#endif