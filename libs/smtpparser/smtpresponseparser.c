#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>

#include "smtprequest.h"
#include "smtpresponse.h"
#include "log.h"
#include "config.h"
#include "smtpresponseparser.h"

int __smtpresponseparser_set_status(smtpresponse_t*, smtpresponseparser_t*);
int __smtpresponseparser_set_message(smtpresponse_t*, smtpresponseparser_t*);
void __smtpresponseparser_flush(smtpresponseparser_t*);


void smtpresponseparser_init(smtpresponseparser_t* parser) {
    parser->stage = SMTPRESPONSEPARSER_STATUS;
    parser->bytes_readed = 0;
    parser->pos_start = 0;
    parser->pos = 0;
    parser->line_size = 0;
    parser->buffer = NULL;
    parser->connection = NULL;

    bufferdata_init(&parser->buf);
}

void smtpresponseparser_set_connection(smtpresponseparser_t* parser, connection_t* connection) {
    parser->connection = connection;
}

void smtpresponseparser_set_buffer(smtpresponseparser_t* parser, char* buffer) {
    parser->buffer = buffer;
}

void smtpresponseparser_free(smtpresponseparser_t* parser) {
    __smtpresponseparser_flush(parser);
    free(parser);
}

void smtpresponseparser_reset(smtpresponseparser_t* parser) {
    __smtpresponseparser_flush(parser);
    smtpresponseparser_init(parser);
}

void __smtpresponseparser_flush(smtpresponseparser_t* parser) {
    if (parser->buf.dynamic_buffer) free(parser->buf.dynamic_buffer);
    parser->buf.dynamic_buffer = NULL;
}

int smtpresponseparser_run(smtpresponseparser_t* parser) {
    smtpresponse_t* response = (smtpresponse_t*)parser->connection->response;
    parser->pos_start = 0;
    parser->pos = 0;

    for (parser->pos = parser->pos_start; parser->pos < parser->bytes_readed; parser->pos++) {
        char ch = parser->buffer[parser->pos];

        switch (parser->stage)
        {
        case SMTPRESPONSEPARSER_STATUS:
            if (ch == ' ' || ch == '-') {
                bufferdata_complete(&parser->buf);
                if (!__smtpresponseparser_set_status(response, parser))
                    return SMTPRESPONSEPARSER_ERROR;

                bufferdata_push(&parser->buf, ch);
                parser->line_size++;

                if (ch == ' ')
                    parser->stage = SMTPRESPONSEPARSER_MESSAGE;
                if (ch == '-')
                    parser->stage = SMTPRESPONSEPARSER_CONTINUE_MESSAGE;

                break;
            }
            else {
                if (parser->line_size > SMTPRESPONSE_MESSAGE_SIZE - 1)
                    return SMTPRESPONSEPARSER_ERROR;

                bufferdata_push(&parser->buf, ch);
                parser->line_size++;
                break;
            }
        case SMTPRESPONSEPARSER_CONTINUE_MESSAGE:
            if (ch == '\n') {
                parser->stage = SMTPRESPONSEPARSER_STATUS;

                bufferdata_push(&parser->buf, ch);
                parser->line_size = 0;
                break;
            }
            else if (ch == '\0') {
                return SMTPRESPONSEPARSER_ERROR;
            }
            else {
                if (parser->line_size > SMTPRESPONSE_MESSAGE_SIZE - 1)
                    return SMTPRESPONSEPARSER_ERROR;

                bufferdata_push(&parser->buf, ch);
                parser->line_size++;
                break;
            }
        case SMTPRESPONSEPARSER_MESSAGE:
            if (ch == '\n') {
                bufferdata_push(&parser->buf, ch);
                parser->line_size++;

                bufferdata_complete(&parser->buf);
                if (!__smtpresponseparser_set_message(response, parser))
                    return SMTPRESPONSEPARSER_ERROR;

                bufferdata_reset(&parser->buf);

                return SMTPRESPONSEPARSER_COMPLETE;
            }
            else if (ch == '\0') {
                return SMTPRESPONSEPARSER_ERROR;
            }
            else {
                if (parser->line_size > SMTPRESPONSE_MESSAGE_SIZE - 1)
                    return SMTPRESPONSEPARSER_ERROR;

                bufferdata_push(&parser->buf, ch);
                parser->line_size++;
                break;
            }
        default:
            return SMTPRESPONSEPARSER_ERROR;
        }
    }

    return SMTPRESPONSEPARSER_CONTINUE;
}

void smtpresponseparser_set_bytes_readed(smtpresponseparser_t* parser, int readed) {
    parser->bytes_readed = readed;
}

int __smtpresponseparser_set_status(smtpresponse_t* response, smtpresponseparser_t* parser) {
    char* string = bufferdata_get(&parser->buf);
    int status = atoi(string);
    if (status == 0)
        return 0;

    response->status = status;

    return 1;
}

int __smtpresponseparser_set_message(smtpresponse_t* response, smtpresponseparser_t* parser) {
    size_t writed = bufferdata_writed(&parser->buf);
    char* string = bufferdata_get(&parser->buf);

    if (string[writed - 2] != '\r' || string[writed - 1] != '\n')
        return 0;

    if (writed > SMTPRESPONSE_MESSAGE_SIZE - 1)
        writed = SMTPRESPONSE_MESSAGE_SIZE - 1;

    strncpy(response->message, string, writed);
    response->message[writed] = 0;

    return 1;
}
