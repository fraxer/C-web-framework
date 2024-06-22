#ifndef __VIEWPARSER__
#define __VIEWPARSER__

#include "viewcommon.h"

typedef enum viewparser_stage {
    VIEWPARSER_TEXT = 0,
    VIEWPARSER_VARIABLE,
    VIEWPARSER_BRANCH,
    VIEWPARSER_CONDITION_EXPR,
    VIEWPARSER_LOOP_ITEM,
    VIEWPARSER_LOOP_INDEX,
    VIEWPARSER_LOOP_IN,
    VIEWPARSER_LOOP_VAR,
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

typedef struct viewparser_context {
    viewparser_stage_e stage;
    viewparser_symbol_e symbol;
    size_t bytes_readed;
    size_t pos;
    char* buffer;

    struct viewparser_context* parent;
    struct viewparser_context* child;
    struct viewparser_context* last_child;
    struct viewparser_context* next;
} viewparser_context_t;

typedef struct {
    const char* storage_name;
    const char* path;
    bufferdata_t variable_buffer;
    int level;

    viewparser_context_t* context;
    viewparser_context_t* current_context;

    view_tag_t* root_tag;
    view_tag_t* current_tag;
} viewparser_t;

viewparser_t* viewparser_init(const char* restrict storage_name, const char* restrict path);
int viewparser_run(viewparser_t* parser);
view_tag_t* viewparser_move_root_tag(viewparser_t* parser);
void viewparser_free(viewparser_t* parser);

#endif
