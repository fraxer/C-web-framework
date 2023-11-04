#include "http1parsercommon.h"

int http1parser_is_ctl(int c) {
    return (c >= 0 && c <= 31) || (c == 127);
}