#ifndef __VIEWCOMMON__
#define __VIEWCOMMON__

#include <stdatomic.h>

#include "json.h"
#include "bufferdata.h"

#define VIEWPARSER_VARIABLE_ITEM_NAME_SIZE 80

typedef enum {
    VIEW_TAGTYPE_VAR = 0,
    VIEW_TAGTYPE_COND,
    VIEW_TAGTYPE_COND_IF,
    VIEW_TAGTYPE_COND_ELSEIF,
    VIEW_TAGTYPE_COND_ELSE,
    VIEW_TAGTYPE_LOOP,
    VIEW_TAGTYPE_INC
} viewparser_tagtype_e;

typedef struct view_variable_index {
    int value;
    struct view_variable_index* next;
} view_variable_index_t;

typedef struct view_variable_item {
    view_variable_index_t* index;
    view_variable_index_t* last_index;
    char name[VIEWPARSER_VARIABLE_ITEM_NAME_SIZE];
    size_t name_length;
    struct view_variable_item* next;
} view_variable_item_t;

typedef struct view_tag {
    int type; // var, cond, loop, inc
    size_t parent_text_offset;
    size_t parent_text_size;
    bufferdata_t result_content;
    struct view_tag* data_parent;
    struct view_tag* parent;
    struct view_tag* child;
    struct view_tag* last_child;
    struct view_tag* next;

    view_variable_item_t* item;
    view_variable_item_t* last_item;

    void(*free)(struct view_tag* tag);
} view_tag_t;

// конкретное условие в условном блоке
typedef struct view_condition_item {
    view_tag_t base;
    int result;
    int reverse;
    int always_true;
} view_condition_item_t;

typedef struct view_loop {
    view_tag_t base;

    char element_name[VIEWPARSER_VARIABLE_ITEM_NAME_SIZE];
    char key_name[VIEWPARSER_VARIABLE_ITEM_NAME_SIZE];
    char key_value[VIEWPARSER_VARIABLE_ITEM_NAME_SIZE];

    jsontok_t* token;
} view_loop_t;

typedef struct view_include {
    view_tag_t base;

    int is_control_key;
    int control_key_index;
} view_include_t;

typedef struct view_loop_copy {
    view_tag_t* source_address;
    view_loop_t tag;
    struct view_loop_copy* next;
} view_loop_copy_t;

typedef struct view_copy_tags {
    view_loop_copy_t* copy;
    view_loop_copy_t* last_copy;
    bufferdata_t buf;
} view_copy_tags_t;

typedef struct view {
    char* path;
    view_tag_t* root_tag;
    struct view* next;
} view_t;

void view_tag_free(view_tag_t* tag);
void view_tag_condition_free(view_tag_t* tag);
void view_tag_loop_free(view_tag_t* tag);
void view_tag_include_free(view_tag_t* tag);

#endif
