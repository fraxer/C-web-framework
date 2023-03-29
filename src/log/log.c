#include <stdarg.h>
#include <stdio.h>
#include <syslog.h>
#include "log.h"

void log_init() {
    openlog(NULL, LOG_CONS | LOG_NDELAY, LOG_USER);
}

void log_close() {
    closelog();
}

void log_reinit() {
    log_close();
    log_init();
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

    vprintf(format, args);

    va_end(args);
    va_start(args, format);

    vsyslog(LOG_ERR, format, args);

    va_end(args);

    return;
}

void log_info(const char* format, ...) {
    va_list args;

    va_start(args, format);

    vprintf(format, args);

    va_end(args);
    va_start(args, format);

    vsyslog(LOG_INFO, format, args);

    va_end(args);

    return;
}
