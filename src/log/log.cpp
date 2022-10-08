#include <stdarg.h>
#include <cstdio>
#include <syslog.h>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/error/en.h"
#include "log.h"

namespace log {

int init() {
    openlog(NULL, LOG_CONS | LOG_NDELAY, LOG_USER);

    return 0;
}

int setParam(char* key, void* value) {
    return 0;
}

int close() {
    closelog();

    return 0;
}

void log(int level, const char* format, va_list* args) {

    vprintf(format, *args);

    vsyslog(level, format, *args);

    return;
}

void log(int level, const char* format, ...) {
    va_list args;

    va_start(args, format);

    log(LOG_ERR, format, &args);

    va_end(args);

    return;
}

void print(const char* format, ...) {
    va_list args;

    va_start(args, format);

    vprintf(format, args);

    va_end(args);

    return;
}

void logError(const char* format, ...) {
    va_list args;

    va_start(args, format);

    log(LOG_ERR, format, &args);

    va_end(args);

    return;
}

} // namespace
