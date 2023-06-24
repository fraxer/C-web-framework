#ifndef __WEBSOCKETS__
#define __WEBSOCKETS__

#include <unistd.h>

#include "connection.h"

void websockets_read(connection_t*, char*, size_t);

void websockets_write(connection_t*, char*, size_t);

ssize_t websockets_read_internal(connection_t*, char*, size_t);

ssize_t websockets_write_internal(connection_t*, const char*, size_t);

#endif
