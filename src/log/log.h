#ifndef __LOG__
#define __LOG__

#include <stdarg.h>

namespace log {

int init();

int setParam(char*, void*);

int close();

void print(const char*, ...);

void log(int, const char*, va_list*);

void log(int, const char*, ...);

void logError(const char*, ...);

} // namespace

#endif