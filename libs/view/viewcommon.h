#ifndef __VIEWCOMMON__
#define __VIEWCOMMON__

#include "json.h"
#include "bufferdata.h"

#define VIEWPARSER_VARIABLE_ITEM_NAME_SIZE 80

typedef enum {
    VIEWPARSER_TAGTYPE_VAR = 0,
    VIEWPARSER_TAGTYPE_COND,
    VIEWPARSER_TAGTYPE_COND_IF,
    VIEWPARSER_TAGTYPE_COND_ELSEIF,
    VIEWPARSER_TAGTYPE_COND_ELSE,
    VIEWPARSER_TAGTYPE_LOOP,
    VIEWPARSER_TAGTYPE_INC
} viewparser_tagtype_e;

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

typedef struct viewparser_tag {
    int type; // var, cond, loop, inc
    size_t parent_text_offset;
    size_t parent_text_size;
    bufferdata_t result_content;
    struct viewparser_tag* data_parent;
    struct viewparser_tag* parent;
    struct viewparser_tag* child;
    struct viewparser_tag* last_child;
    struct viewparser_tag* next;

    viewparser_variable_item_t* item;
    viewparser_variable_item_t* last_item;
} viewparser_tag_t;

// конкретное условие в условном блоке
typedef struct viewparser_condition_item {
    viewparser_tag_t base;
    int result;
    int reverse;
    int always_true;
} viewparser_condition_item_t;

typedef struct viewparser_loop {
    viewparser_tag_t base;

    char element_name[VIEWPARSER_VARIABLE_ITEM_NAME_SIZE];
    char key_name[VIEWPARSER_VARIABLE_ITEM_NAME_SIZE];
    char key_value[VIEWPARSER_VARIABLE_ITEM_NAME_SIZE];

    jsontok_t* token;
} viewparser_loop_t;

typedef struct viewparser_include {
    viewparser_tag_t base;

    int is_control_key;
    int control_key_index;
} viewparser_include_t;

typedef struct view_loop_copy {
    viewparser_tag_t* source_address;
    viewparser_loop_t tag;
    struct view_loop_copy* next;
} view_loop_copy_t;

typedef struct view_copy_tags {
    view_loop_copy_t* copy;
    view_loop_copy_t* last_copy;
} view_copy_tags_t;

typedef struct view {
    bufferdata_t buf;
    char* path;
    viewparser_tag_t* root_tag;
    struct view* next;
} view_t;

#endif
