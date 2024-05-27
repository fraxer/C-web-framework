#ifndef __VIEWPARSER__
#define __VIEWPARSER__

#include "json.h"
#include "bufferdata.h"

#define VIEWPARSER_VARIABLE_ITEM_NAME_SIZE 80

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
    VIEWPARSER_VARIABLE_SPACE,
    VIEWPARSER_VARIABLE_BRACKETOPEN,
    VIEWPARSER_VARIABLE_BRACKETCLOSE,
    VIEWPARSER_PERCENT,
} viewparser_symbol_e;

typedef enum {
    VIEWPARSER_TAGTYPE_VAR = 0,
    VIEWPARSER_TAGTYPE_COND,
    VIEWPARSER_TAGTYPE_LOOP,
    VIEWPARSER_TAGTYPE_INC
} viewparser_tagtype_e;

typedef struct viewparser_tag {
    int type; // var, cond, loop, inc
    size_t parent_position; // position in parent content tag
    bufferdata_t result_content;
    jsontok_t* json_token;
    struct viewparser_tag* parent;
    struct viewparser_tag* child;
    struct viewparser_tag* last_child;
    struct viewparser_tag* next;
} viewparser_tag_t;

typedef struct viewparser_variable_index {
    int value;
    struct viewparser_variable_index* next;
} viewparser_variable_index_t;

typedef struct viewparser_variable_item {
    viewparser_variable_index_t* index;
    viewparser_variable_index_t* last_index;
    char name[VIEWPARSER_VARIABLE_ITEM_NAME_SIZE];
    size_t name_length;
    struct viewparser_variable_item* next;
} viewparser_variable_item_t;

typedef struct viewparser_variable {
    viewparser_tag_t base;

    viewparser_variable_item_t* item;
    viewparser_variable_item_t* last_item;
} viewparser_variable_t;

// конкретное условие в условном блоке
typedef struct viewparser_condition_item {
    viewparser_tag_t base;
    int reverse;
    int always_true;

    viewparser_variable_item_t* item;
    viewparser_variable_item_t* last_item;
} viewparser_condition_item_t;

// обертка для условного блока
typedef struct viewparser_condition {
    viewparser_tag_t base;

    viewparser_condition_item_t* item;
    viewparser_condition_item_t* last_item;
} viewparser_condition_t;

typedef struct viewparser_loop {
    viewparser_tag_t base;

    char* element_name;
    char* key_name;

    viewparser_variable_item_t* item;
    viewparser_variable_item_t* last_item;
} viewparser_loop_t;

typedef struct viewparser_include {
    viewparser_tag_t base;

    // char* path; // variable_buffer use
} viewparser_include_t;

typedef struct viewparser_context {
    viewparser_stage_e stage;
    viewparser_symbol_e symbol;
    size_t bytes_readed;
    size_t pos_start;
    size_t pos;
    char* buffer;

    struct viewparser_context* parent;
    struct viewparser_context* childs;
} viewparser_context_t;

typedef struct {
    const jsondoc_t* document;
    const char* storage_name;
    const char* path;
    bufferdata_t variable_buffer;
    bufferdata_t buf;

    viewparser_context_t* context;
    viewparser_context_t* current_context;

    viewparser_tag_t* root_tag;
    viewparser_tag_t* current_tag;
} viewparser_t;

viewparser_t* viewparser_init(const jsondoc_t* document, const char* storage_name, const char* path);
int viewparser_run(viewparser_t* parser);
char* viewparser_get_content(viewparser_t* parser);
void viewparser_free(viewparser_t* parser);


#endif
