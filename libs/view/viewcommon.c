#include <stdlib.h>

#include "viewcommon.h"

void __view_tag_free(viewparser_tag_t* tag);
void __view_variable_items_free(viewparser_variable_item_t* item);
void __view_variable_indexes_free(viewparser_variable_index_t* item_index);

void view_tag_free(viewparser_tag_t* tag) {
    if (tag == NULL) return;

    __view_tag_free(tag);

    free(tag);
}

void view_tag_condition_free(viewparser_tag_t* tag) {
    if (tag == NULL) return;

    __view_tag_free(tag);

    free((viewparser_condition_item_t*)tag);
}

void view_tag_loop_free(viewparser_tag_t* tag) {
    if (tag == NULL) return;

    __view_tag_free(tag);

    free((viewparser_loop_t*)tag);
}

void view_tag_include_free(viewparser_tag_t* tag) {
    if (tag == NULL) return;

    __view_tag_free(tag);

    free((viewparser_include_t*)tag);
}

void __view_tag_free(viewparser_tag_t* tag) {
    tag->type = VIEWPARSER_TAGTYPE_VAR;
    tag->parent_text_offset = 0;
    tag->parent_text_size = 0;
    bufferdata_clear(&tag->result_content);
    tag->data_parent = NULL;
    tag->parent = NULL;
    tag->last_child = NULL;
    tag->last_item = NULL;

    if (tag->child != NULL)
        tag->child->free(tag->child);

    if (tag->next != NULL)
        tag->next->free(tag->next);

    __view_variable_items_free(tag->item);
}

void __view_variable_items_free(viewparser_variable_item_t* var_item) {
    viewparser_variable_item_t* item = var_item;
    while (item != NULL) {
        viewparser_variable_item_t* next = item->next;
        __view_variable_indexes_free(item->index);
        free(item);
        item = next;
    }
}

void __view_variable_indexes_free(viewparser_variable_index_t* item_index) {
    viewparser_variable_index_t* index = item_index;
    while (index != NULL) {
        viewparser_variable_index_t* next = index->next;
        free(index);
        index = next;
    }
}
