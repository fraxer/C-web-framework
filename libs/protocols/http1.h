#ifndef __HTTP1__
#define __HTTP1__

#include "connection.h"

void http1_read(connection_t*, char*, size_t);

void http1_write(connection_t*, char*, size_t);

ssize_t http1_read_internal(connection_t*, char*, size_t);

ssize_t http1_write_internal(connection_t*, const char*, size_t);

ssize_t http1_write_chunked(connection_t*, const char*, size_t, int);

#endif
