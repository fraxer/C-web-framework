#ifndef __WEBSOCKETSINTERNAL__
#define __WEBSOCKETSINTERNAL__

#include <unistd.h>

#include "connection.h"

void websockets_wrap_read(connection_t*, char*, size_t);
void websockets_wrap_write(connection_t*, char*, size_t);

#endif
