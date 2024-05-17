#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <linux/limits.h>

#include "viewparser.h"
#include "view.h"

char* __render(jsondoc_t* document, const char* storage_name, const char* path);

char* render(jsondoc_t* document, const char* storage_name, const char* path_format, ...) {
    char path[PATH_MAX];

    va_list args;
    va_start(args, path_format);
    vsnprintf(path, sizeof(path), path_format, args);
    va_end(args);

    return __render(document, storage_name, path);
}

char* __render(jsondoc_t* document, const char* storage_name, const char* path) {
    viewparser_t* parser = viewparser_init(document, storage_name, path);
    if (parser == NULL)
        return NULL;

    char* content = NULL;
    if (viewparser_run(parser))
        content = viewparser_get_content(parser);

    viewparser_free(parser);

    return content;
}
