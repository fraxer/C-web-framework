#include <string.h>

#include "smtpinternal.h"
#include "smtprequest.h"
#include "smtpresponse.h"
#include "smtpresponseparser.h"

ssize_t __smtp_read_internal(connection_t* connection, char* buffer, size_t size);
ssize_t __smtp_write_internal(connection_t* connection, const char* buffer, size_t size);

void smtp_client_read(connection_t* connection, char* buffer, size_t buffer_size) {
    smtpresponseparser_t* parser = ((smtpresponse_t*)connection->response)->parser;
    smtpresponseparser_set_connection(parser, connection);
    smtpresponseparser_set_buffer(parser, buffer);

    while (1) {
        int bytes_readed = __smtp_read_internal(connection, buffer, buffer_size);
        switch (bytes_readed) {
        case -1:
            connection->after_read_request(connection);
            return;
        case 0:
            connection->keepalive_enabled = 0;
            connection->after_read_request(connection);
            return;
        default:
            smtpresponseparser_set_bytes_readed(parser, bytes_readed);

            switch (smtpresponseparser_run(parser)) {
            case SMTPRESPONSEPARSER_ERROR:
                // smtpresponse_default((smtpresponse_t*)connection->response, 500);
                connection->after_read_request(connection);
                return;
            case SMTPRESPONSEPARSER_CONTINUE:
                break;
            case SMTPRESPONSEPARSER_COMPLETE:
                smtpresponseparser_reset(parser);
                connection->after_read_request(connection);
                return;
            }
        }
    }
}

void smtp_client_write_command(connection_t* connection, char* buffer, size_t buffer_size) {
    (void)buffer;
    (void)buffer_size;

    smtprequest_t* request = (smtprequest_t*)connection->request;

    (void)__smtp_write_internal(connection, request->command, strlen(request->command));

    connection->after_write_request(connection);
}

void smtp_client_write_content(connection_t* connection, char* buffer, size_t buffer_size) {
    (void)buffer;
    (void)buffer_size;

    smtprequest_data_t* request = (smtprequest_data_t*)connection->request;

    (void)__smtp_write_internal(connection, request->content, request->content_size);

    connection->after_write_request(connection);
}

ssize_t __smtp_read_internal(connection_t* connection, char* buffer, size_t size) {
    return connection->ssl ?
        openssl_read(connection->ssl, buffer, size) :
        recv(connection->fd, buffer, size, 0);
}

ssize_t __smtp_write_internal(connection_t* connection, const char* buffer, size_t size) {
    return connection->ssl ?
        openssl_write(connection->ssl, buffer, size) :
        send(connection->fd, buffer, size, MSG_NOSIGNAL);
}
