#include <stdarg.h>
#include <stdio.h>
#include <syslog.h>
#include "log.h"

void* log_init() {
    openlog(NULL, LOG_CONS | LOG_NDELAY, LOG_USER);

    return NULL;
}

int log_close() {
    closelog();

    return 0;
}

void log_full(int level, const char* format, va_list* args) {

    vprintf(format, *args);

    vsyslog(level, format, *args);

    return;
}

void log_print(const char* format, ...) {
    va_list args;

    va_start(args, format);

    vprintf(format, args);

    va_end(args);

    return;
}

void log_error(const char* format, ...) {
    va_list args;

    va_start(args, format);

    log_full(LOG_ERR, format, &args);

    va_end(args);

    return;
}

void log_info(const char* format, ...) {
    va_list args;

    va_start(args, format);

    log_full(LOG_INFO, format, &args);

    va_end(args);

    return;
}
