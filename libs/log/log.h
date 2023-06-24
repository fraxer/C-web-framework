#ifndef __LOG__
#define __LOG__

#include <stdarg.h>

void log_init();

void log_close();

void log_reinit();

void log_print(const char*, ...);

void log_error(const char*, ...);

void log_info(const char*, ...);

#endif