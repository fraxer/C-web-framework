#ifndef __SMTPINTERNAL__
#define __SMTPINTERNAL__

#include "connection.h"

void smtp_client_read(connection_t* connection, char* buffer, size_t buffer_size);
void smtp_client_write_command(connection_t* connection, char* buffer, size_t buffer_size);
void smtp_client_write_content(connection_t* connection, char* buffer, size_t buffer_size);

#endif
