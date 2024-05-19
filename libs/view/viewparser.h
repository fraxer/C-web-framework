#ifndef __VIEWPARSER__
#define __VIEWPARSER__

#include "json.h"
#include "bufferdata.h"

// typedef struct viewparser_content {
//     char* content;
//     struct viewparser_content* next;
// } viewparser_content_t;
#define VIEWPARSER_VARIABLE_ITEM_NAME_SIZE 80

typedef struct viewparser_variable_item {
    int index;
    char name[VIEWPARSER_VARIABLE_ITEM_NAME_SIZE];
    size_t name_length;
    struct viewparser_variable_item* next;
} viewparser_variable_item_t;

typedef struct viewparser_variable {
    jsontok_t* json_token;
    viewparser_variable_item_t* item;
    viewparser_variable_item_t* last_item;
    struct viewparser_variable* next;
} viewparser_variable_t;

typedef enum viewparser_stage {
    VIEWPARSER_TEXT = 0,
    VIEWPARSER_VARIABLE,
    VIEWPARSER_BRANCH,
    VIEWPARSER_INCLUDE,
} viewparser_stage_e;

typedef enum viewparser_symbol {
    VIEWPARSER_DEFAULT = 0,
    VIEWPARSER_FIGFIGOPEN,
    VIEWPARSER_FIGFIGCLOSE,
    VIEWPARSER_FIGPERCENTOPEN,
    VIEWPARSER_FIGPERCENTCLOSE,
    VIEWPARSER_FIGASTOPEN,
    VIEWPARSER_FIGASTCLOSE,
    VIEWPARSER_VARIABLE_DOT,
} viewparser_symbol_e;

typedef struct {
    const jsondoc_t* document;
    const char* storage_name;
    const char* path;
    bufferdata_t variable_buffer;

    char* buffer;
    viewparser_stage_e stage;
    viewparser_symbol_e symbol;
    bufferdata_t buf;
    size_t bytes_readed;
    size_t pos_start;
    size_t pos;
    viewparser_variable_t* variable;
    viewparser_variable_t* last_variable;
} viewparser_t;

viewparser_t* viewparser_init(const jsondoc_t* document, const char* storage_name, const char* path);
int viewparser_run(viewparser_t* parser);
char* viewparser_get_content(viewparser_t* parser);
void viewparser_free(viewparser_t* parser);


#endif
