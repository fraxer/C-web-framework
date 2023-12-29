#ifndef __HTTP1INTERNAL__
#define __HTTP1INTERNAL__

#include "connection.h"

void http1_wrap_read(connection_t*, char*, size_t);
void http1_wrap_write(connection_t*, char*, size_t);

#endif
