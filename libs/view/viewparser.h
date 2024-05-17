#ifndef __VIEWPARSER__
#define __VIEWPARSER__

#include "json.h"

typedef struct {
    const jsondoc_t* document;
    const char* storage_name;
    const char* path;
    char* content;
} viewparser_t;

viewparser_t* viewparser_init(const jsondoc_t* document, const char* storage_name, const char* path);
int viewparser_run(viewparser_t* parser);
char* viewparser_get_content(viewparser_t* parser);
void viewparser_free(viewparser_t* parser);


#endif
