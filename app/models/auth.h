#ifndef __AUTH__
#define __AUTH__

#include "http1.h"
#include "user.h"

user_t* authenticate(const char* dbinstance, const char* username, const char* password);
user_t* authenticate_by_token(const char* dbinstance, http1request_t* request);

#endif
