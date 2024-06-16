#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <linux/limits.h>

#include "viewparser.h"
#include "viewstore.h"
#include "view.h"

char* __view_render(jsondoc_t* document, const char* storage_name, const char* path);
char* __view_make_content(view_t* view, jsondoc_t* document);
int __view_get_condition_value(view_copy_tags_t* copy_tags, jsondoc_t* document, viewparser_tag_t* tag);
const char* __view_get_tag_value(view_copy_tags_t* copy_tags, jsondoc_t* document, viewparser_tag_t* tag);
const jsontok_t* __view_get_loop_value(view_copy_tags_t* copy_tags, jsondoc_t* document, viewparser_tag_t* tag);
void __view_build_content_recursive(view_t* view, jsondoc_t* document, view_copy_tags_t* copy_tags, viewparser_tag_t* tag);
viewparser_loop_t* __view_copy_loop_tag(view_copy_tags_t* copy_tags, viewparser_tag_t* tag);
viewparser_loop_t* __view_get_copy_loop_tag(view_copy_tags_t* copy_tags, viewparser_tag_t* tag);

char* view_render(jsondoc_t* document, const char* storage_name, const char* path_format, ...) {
    char path[PATH_MAX];

    va_list args;
    va_start(args, path_format);
    vsnprintf(path, sizeof(path), path_format, args);
    va_end(args);

    return __view_render(document, storage_name, path);
}

char* __view_render(jsondoc_t* document, const char* storage_name, const char* path) {
    view_t* view = viewstore_get_view(path);
    if (view == NULL) {
        viewparser_t* parser = viewparser_init(storage_name, path);
        if (parser == NULL)
            return NULL;

        if (!viewparser_run(parser)) {
            viewparser_free(parser);
            return NULL;
        }

        view = viewstore_add_view(viewparser_move_root_tag(parser), path);

        viewparser_free(parser);

        if (view == NULL)
            return NULL;
    }

    return __view_make_content(view, document);
}

char* __view_make_content(view_t* view, jsondoc_t* document) {
    bufferdata_reset(&view->buf);

    view_copy_tags_t copy_tags;
    copy_tags.copy = NULL;
    copy_tags.last_copy = NULL;

    viewparser_tag_t* child = view->root_tag->child;
    if (child == NULL) {
        const size_t size = bufferdata_writed(&view->root_tag->result_content);
        const char* content = bufferdata_get(&view->root_tag->result_content);

        for (size_t i = 0; i < size; i++)
            bufferdata_push(&view->buf, content[i]);
    }
    else
        __view_build_content_recursive(view, document, &copy_tags, child);

    // free copy tags

    bufferdata_complete(&view->buf);

    char* data = bufferdata_copy(&view->buf);

    bufferdata_clear(&view->buf);

    return data;
}

int __view_get_condition_value(view_copy_tags_t* copy_tags, jsondoc_t* document, viewparser_tag_t* tag) {
    viewparser_variable_item_t* item = tag->item;
    viewparser_loop_t* tag_for = __view_get_copy_loop_tag(copy_tags, tag->data_parent);
    const jsontok_t* json_token = json_root(document);
    if (tag_for != NULL) {
        viewparser_loop_t* tag_parent = tag_for;

        while (tag_parent != NULL) {
            if (strcmp(item->name, tag_parent->element_name) == 0) {
                json_token = tag_parent->token;

                if (item->next != NULL)
                    item = item->next;

                break;
            }

            tag_parent = __view_get_copy_loop_tag(copy_tags, tag_parent->base.data_parent);
        }
    }

    while (item) {
        const jsontok_t* token = NULL;
        if (json_token->type == JSON_STRING)
            token = json_token;
        else if (json_token->type == JSON_OBJECT)
            token = json_object_get(json_token, item->name);

        if (token == NULL) break;
        
        if (token->type == JSON_ARRAY) {
            int stop = 0;
            viewparser_variable_index_t* index = item->index;
            while (index) {
                token = json_array_get(token, index->value);
                stop = token == NULL;
                if (stop) break;

                index = index->next;
            }

            if (stop) break;
        }

        if (item->next == NULL) {
            if (json_is_bool(token))
                return json_bool(token);
        }
        else {
            json_token = token;
        }

        item = item->next;
    }

    return 0;
}

const char* __view_get_tag_value(view_copy_tags_t* copy_tags, jsondoc_t* document, viewparser_tag_t* tag) {
    viewparser_variable_item_t* item = tag->item;
    viewparser_loop_t* tag_for = __view_get_copy_loop_tag(copy_tags, tag->data_parent);
    const jsontok_t* json_token = json_root(document);
    if (tag_for != NULL) {
        viewparser_loop_t* tag_parent = tag_for;

        while (tag_parent != NULL) {
            if (item->next == NULL && strcmp(item->name, tag_parent->key_name) == 0)
                return tag_parent->key_value;

            if (strcmp(item->name, tag_parent->element_name) == 0) {
                json_token = tag_parent->token;

                if (item->next != NULL)
                    item = item->next;

                break;
            }

            tag_parent = __view_get_copy_loop_tag(copy_tags, tag_parent->base.data_parent);
        }
    }

    while (item) {
        const jsontok_t* token = NULL;
        if (json_token->type == JSON_STRING)
            token = json_token;
        else if (json_token->type == JSON_OBJECT)
            token = json_object_get(json_token, item->name);

        if (token == NULL) break;
        
        if (token->type == JSON_ARRAY) {
            int stop = 0;
            viewparser_variable_index_t* index = item->index;
            while (index) {
                token = json_array_get(token, index->value);
                stop = token == NULL;
                if (stop) break;

                index = index->next;
            }

            if (stop) break;
        }

        if (item->next == NULL) {
            if (json_is_string(token))
                return json_string(token);
        }
        else {
            json_token = token;
        }

        item = item->next;
    }

    return NULL;
}

const jsontok_t* __view_get_loop_value(view_copy_tags_t* copy_tags, jsondoc_t* document, viewparser_tag_t* tag) {
    viewparser_variable_item_t* item = tag->item;
    viewparser_loop_t* tag_for = __view_get_copy_loop_tag(copy_tags, tag->data_parent);
    const jsontok_t* json_token = json_root(document);
    if (tag_for != NULL) {
        viewparser_loop_t* tag_parent = tag_for;

        while (tag_parent != NULL) {
            if (strcmp(item->name, tag_parent->element_name) == 0) {
                json_token = tag_parent->token;

                if (item->next != NULL)
                    item = item->next;

                break;
            }

            tag_parent = __view_get_copy_loop_tag(copy_tags, tag_parent->base.data_parent);
        }
    }

    while (item) {
        const jsontok_t* token = NULL;
        if (json_token->type == JSON_ARRAY)
            token = json_token;
        else
            token = json_object_get(json_token, item->name);

        if (token == NULL) break;

        if (token->type == JSON_ARRAY) {
            int stop = 0;
            viewparser_variable_index_t* index = item->index;
            while (index) {
                token = json_array_get(token, index->value);
                stop = token == NULL;
                if (stop) break;

                index = index->next;
            }

            if (stop) break;
        }

        if (item->next == NULL) {
            // convert token value to string
            if (json_is_array(token) || json_is_object(token))
                return token;

            return NULL;
        }
        else {
            json_token = token;
        }

        item = item->next;
    }

    return NULL;
}

void __view_build_content_recursive(view_t* view, jsondoc_t* document, view_copy_tags_t* copy_tags, viewparser_tag_t* tag) {
    viewparser_tag_t* child = tag;
    while (child) {
        switch (child->type) {
        case VIEWPARSER_TAGTYPE_VAR:
        {
            viewparser_tag_t* parent = child->parent;

            size_t size = bufferdata_writed(&parent->result_content);
            const char* content = bufferdata_get(&parent->result_content);
            for (size_t i = child->parent_text_offset; i < child->parent_text_offset + child->parent_text_size && i < size; i++)
                bufferdata_push(&view->buf, content[i]);

            content = __view_get_tag_value(copy_tags, document, child);
            if (content != NULL) {
                size = strlen(content);
                for (size_t i = 0; i < size; i++)
                    bufferdata_push(&view->buf, content[i]);
            }

            if (child->next == NULL) {
                size = bufferdata_writed(&parent->result_content);
                content = bufferdata_get(&parent->result_content);
                for (size_t i = child->parent_text_offset + child->parent_text_size; i < size; i++)
                    bufferdata_push(&view->buf, content[i]);

                if (child == tag) return;
            }

            child = child->next;

            break;
        }
        case VIEWPARSER_TAGTYPE_COND:
        {
            __view_build_content_recursive(view, document, copy_tags, child->child);

            child = child->next;

            break;
        }
        case VIEWPARSER_TAGTYPE_COND_IF:
        case VIEWPARSER_TAGTYPE_COND_ELSEIF:
        case VIEWPARSER_TAGTYPE_COND_ELSE:
        {
            viewparser_tag_t* parent = child->parent;
            viewparser_condition_item_t* tag = (viewparser_condition_item_t*)child;

            const int result = __view_get_condition_value(copy_tags, document, child);
            const int istrue = tag->always_true || (result && !tag->reverse) || (!result && tag->reverse);

            if (!istrue) {
                if (child->next != NULL) {
                    child = child->next;
                    break;
                }
            }

            size_t size = bufferdata_writed(&parent->parent->result_content);
            const char* content = bufferdata_get(&parent->parent->result_content);
            for (size_t i = parent->parent_text_offset; i < parent->parent_text_offset + parent->parent_text_size; i++)
                bufferdata_push(&view->buf, content[i]);

            if (istrue) {
                if (child->child != NULL) {
                    __view_build_content_recursive(view, document, copy_tags, child->child);
                }
                else {
                    const size_t size = bufferdata_writed(&child->result_content);
                    const char* content = bufferdata_get(&child->result_content);
                    for (size_t i = 0; i < size; i++)
                        bufferdata_push(&view->buf, content[i]);
                }
            }

            if (parent->next == NULL) {
                size = bufferdata_writed(&parent->parent->result_content);
                content = bufferdata_get(&parent->parent->result_content);
                for (size_t i = parent->parent_text_offset + parent->parent_text_size; i < size; i++)
                    bufferdata_push(&view->buf, content[i]);
            }

            if (istrue) return;

            child = child->next;

            break;
        }
        case VIEWPARSER_TAGTYPE_LOOP:
        {
            viewparser_loop_t* tag_for = __view_get_copy_loop_tag(copy_tags, child);
            if (tag_for == NULL)
                tag_for = __view_copy_loop_tag(copy_tags, child);

            if (tag_for == NULL)
                return;

            viewparser_tag_t* parent = child->parent;
            size_t size = bufferdata_writed(&parent->result_content);
            const char* content = bufferdata_get(&parent->result_content);
            for (size_t i = child->parent_text_offset; i < child->parent_text_offset + child->parent_text_size && i < size; i++)
                bufferdata_push(&view->buf, content[i]);

            const jsontok_t* token = __view_get_loop_value(copy_tags, document, child);
            if (token != NULL) {
                if (token->type == JSON_OBJECT || token->type == JSON_ARRAY) {
                    for (jsonit_t it = json_init_it(token); !json_end_it(&it); it = json_next_it(&it)) {
                        if (token->type == JSON_OBJECT)
                            sprintf(tag_for->key_value, "%s", (char*)json_it_key(&it));
                        else
                            sprintf(tag_for->key_value, "%d", (*(int*)json_it_key(&it)));

                        tag_for->token = json_it_value(&it);

                        if (child->child != NULL) {
                            __view_build_content_recursive(view, document, copy_tags, child->child);
                        }
                        else {
                            const size_t size = bufferdata_writed(&child->result_content);
                            const char* content = bufferdata_get(&child->result_content);
                            for (size_t i = 0; i < size; i++)
                                bufferdata_push(&view->buf, content[i]);
                        }
                    }
                }
            }

            if (child->next == NULL) {
                size = bufferdata_writed(&parent->result_content);
                content = bufferdata_get(&parent->result_content);
                for (size_t i = child->parent_text_offset + child->parent_text_size; i < size; i++)
                    bufferdata_push(&view->buf, content[i]);
            }

            child = child->next;

            break;
        }
        case VIEWPARSER_TAGTYPE_INC:
        {
            viewparser_tag_t* parent = child->parent;

            if (parent != NULL) {
                const char* content = bufferdata_get(&parent->result_content);
                for (size_t i = child->parent_text_offset; i < child->parent_text_offset + child->parent_text_size; i++)
                    bufferdata_push(&view->buf, content[i]);
            }

            if (child->child != NULL) {
                __view_build_content_recursive(view, document, copy_tags, child->child);
            }
            else {
                const size_t size = bufferdata_writed(&child->result_content);
                const char* content = bufferdata_get(&child->result_content);
                for (size_t i = 0; i < size; i++)
                    bufferdata_push(&view->buf, content[i]);
            }

            if (child->next == NULL && parent != NULL) {
                const size_t size = bufferdata_writed(&parent->result_content);
                const char* content = bufferdata_get(&parent->result_content);
                for (size_t i = child->parent_text_offset + child->parent_text_size; i < size; i++)
                    bufferdata_push(&view->buf, content[i]);
            }

            child = child->next;

            break;
        }
        }
    }
}

viewparser_loop_t* __view_copy_loop_tag(view_copy_tags_t* copy_tags, viewparser_tag_t* tag) {
    view_loop_copy_t* copy = malloc(sizeof * copy);
    if (copy == NULL) return NULL;

    copy->source_address = (viewparser_tag_t*)tag;
    memcpy(&copy->tag, tag, sizeof(viewparser_loop_t));
    copy->next = NULL;

    if (copy_tags->copy == NULL)
        copy_tags->copy = copy;

    if (copy_tags->last_copy != NULL)
        copy_tags->last_copy->next = copy;

    copy_tags->last_copy = copy;

    return &copy->tag;
}

viewparser_loop_t* __view_get_copy_loop_tag(view_copy_tags_t* copy_tags, viewparser_tag_t* tag) {
    if (tag == NULL) return NULL;

    view_loop_copy_t* copy = copy_tags->copy;
    while (copy) {
        if (copy->source_address == tag) {
            return &copy->tag;
        }

        copy = copy->next;
    }

    return NULL;
}
