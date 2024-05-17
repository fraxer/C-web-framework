#include "storage.h"
#include "viewparser.h"

viewparser_t* viewparser_init(const jsondoc_t* document, const char* storage_name, const char* path) {
    viewparser_t* parser = malloc(sizeof * parser);
    if (parser == NULL) return NULL;

    parser->document = document;
    parser->storage_name = storage_name;
    parser->path = path;
    parser->content = NULL;

    return parser;
}

int viewparser_run(viewparser_t* parser) {
    file_t file = storage_file_get(parser->storage_name, parser->path);
    if (!file.ok)
        return 0;

    char* file_content = file.content(&file);
    // if (file_content != NULL) {
        
    // }

    file.close(&file);
    
    parser->content = file_content;

    return 1;
}

char* viewparser_get_content(viewparser_t* parser) {
    return parser->content;
}

void viewparser_free(viewparser_t* parser) {
    if (parser == NULL)
        return;

    free(parser);
}
