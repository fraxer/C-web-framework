#ifndef __LISTENTER__
#define __LISTENTER__

#include "connection.h"

void listener_read(connection_t*, char*, size_t);
int listener_connection_close(connection_t*);

#endif
