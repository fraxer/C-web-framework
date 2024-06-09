#include "storage.h"
#include "log.h"
#include "viewparser.h"

static viewparser_context_t* __viewparser_context_create();
static int __viewparser_readfile(viewparser_context_t* context, const char* storage_name,const char* path);
static int __viewparser_parse_content(viewparser_t* parser);
static int __viewparser_variable_create(viewparser_t* parser);
static void __viewparser_variable_free(viewparser_variable_t* variable);
static viewparser_variable_item_t* __viewparser_variable_item_create();
static int __viewparser_variable_item_append(viewparser_variable_item_t* variable_item, viewparser_tag_t* variable);
static void __viewparser_variable_item_free(viewparser_variable_item_t* item);
static int __viewparser_variable_item_complete_name(viewparser_t* parser);
static int __viewparser_variable_item_complete_index(viewparser_t* parser);
static int __viewparser_replace_tag_on_value(viewparser_t* parser);
static int __viewparser_variable_item_index_create(viewparser_variable_item_t* item, int value);
static int __viewparser_variable_write_body(viewparser_t* parser, char ch);

static int __viewparser_tag_init(viewparser_tag_t* tag, int type, viewparser_tag_t* data_parent, viewparser_tag_t* parent);
static viewparser_tag_t* __viewparser_root_tag_create();
static int __viewparser_include_create(viewparser_t* parser);
static int __viewparser_include_write_body(viewparser_t* parser, char ch);
static int __viewparser_include_load_content(viewparser_t* parser);

static int __viewparser_branch_write_body(viewparser_t* parser, char ch);
static int __viewparser_condition_if_create(viewparser_t* parser);
static int __viewparser_condition_elseif_create(viewparser_t* parser);
static int __viewparser_condition_else_create(viewparser_t* parser);
static int __viewparser_condition_item_append(viewparser_variable_item_t* variable_item, viewparser_condition_item_t* tag_if);
static int __viewparser_condition_item_complete_name(viewparser_t* parser);
static int __viewparser_replace_condition_on_value(viewparser_t* parser);

static void __viewparser_build_content(viewparser_t* parser);
static void __viewparser_build_content_recursive(viewparser_t* parser, viewparser_tag_t* child);
static const char* __viewparser_get_tag_value(viewparser_t* parser, viewparser_tag_t* tag);

static char __word_include[8] = {'i', 'n', 'c', 'l', 'u', 'd', 'e', 0};
static char __word_if[3] = {'i', 'f', 0};
static char __word_elseif[7] = {'e', 'l', 's', 'e', 'i', 'f', 0};
static char __word_else[5] = {'e', 'l', 's', 'e', 0};
static char __word_endif[6] = {'e', 'n', 'd', 'i', 'f', 0};
static char __word_for[4] = {'f', 'o', 'r', 0};
static char __word_endfor[7] = {'e', 'n', 'd', 'f', 'o', 'r', 0};

viewparser_t* viewparser_init(const jsondoc_t* document, const char* storage_name, const char* path) {
    viewparser_t* parser = malloc(sizeof * parser);
    if (parser == NULL) return NULL;

    parser->document = document;
    parser->storage_name = storage_name;
    parser->path = path;
    parser->context = NULL;
    parser->current_context = NULL;
    parser->root_tag = NULL;
    parser->current_tag = NULL;

    bufferdata_init(&parser->variable_buffer);
    bufferdata_init(&parser->buf);

    return parser;
}

int viewparser_run(viewparser_t* parser) {
    parser->context = __viewparser_context_create();
    if (parser->context == NULL) return 0;

    parser->root_tag = __viewparser_root_tag_create();
    if (parser->root_tag == NULL) return 0;

    parser->current_tag = parser->root_tag;
    parser->current_context = parser->context;

    if (!__viewparser_readfile(parser->current_context, parser->storage_name, parser->path)) {
        log_error("View file not found %s", parser->path);
        return 0;
    }

    __viewparser_parse_content(parser);

    return 1;
}

char* viewparser_get_content(viewparser_t* parser) {
    bufferdata_reset(&parser->buf);

    __viewparser_build_content(parser);

    bufferdata_complete(&parser->buf);

    return bufferdata_copy(&parser->buf);
}

void viewparser_free(viewparser_t* parser) {
    if (parser == NULL)
        return;

    if (parser->buf.dynamic_buffer) free(parser->buf.dynamic_buffer);
    parser->buf.dynamic_buffer = NULL;

    bufferdata_reset(&parser->buf);

    __viewparser_variable_free(NULL);

    free(parser);
}

viewparser_context_t* __viewparser_context_create() {
    viewparser_context_t* context = malloc(sizeof * context);
    if (context == NULL) return NULL;

    context->stage = VIEWPARSER_TEXT;
    context->symbol = VIEWPARSER_DEFAULT;
    context->bytes_readed = 0;
    // context->pos_start = 0;
    context->pos = 0;
    context->buffer = NULL;
    context->parent = NULL;
    context->child = NULL;
    context->last_child = NULL;
    context->next = NULL;

    return context;
}

int __viewparser_readfile(viewparser_context_t* context, const char* storage_name,const char* path) {
    file_t file = storage_file_get(storage_name, path);
    if (!file.ok)
        return 0;

    context->buffer = file.content(&file);
    context->bytes_readed = file.size;

    file.close(&file);

    // if file empty
    // if (context->buffer == NULL)
    //     return 0;

    return 1;
}

int __viewparser_parse_content(viewparser_t* parser) {
    start:

    for (; parser->current_context->pos < parser->current_context->bytes_readed; parser->current_context->pos++) {
        viewparser_context_t* context = parser->current_context;
        const char ch = context->buffer[context->pos];

        switch (context->stage)
        {
        case VIEWPARSER_TEXT:
        {
            switch (ch)
            {
            case '{':
            {
                if (context->symbol == VIEWPARSER_FIGFIGOPEN) {
                    context->stage = VIEWPARSER_VARIABLE;
                    bufferdata_pop_back(&parser->current_tag->result_content);

                    if (!__viewparser_variable_create(parser))
                        return 0;

                    viewparser_variable_item_t* variable_item = __viewparser_variable_item_create();
                    if (variable_item == NULL)
                        return 0;

                    if (!__viewparser_variable_item_append(variable_item, parser->current_tag))
                        return 0;
                    
                    break;
                }

                context->symbol = VIEWPARSER_FIGFIGOPEN;

                bufferdata_push(&parser->current_tag->result_content, ch);
                break;
            }
            case '*':
            {
                if (context->symbol != VIEWPARSER_FIGFIGOPEN) {
                    bufferdata_push(&parser->current_tag->result_content, ch);
                    break;
                }

                context->stage = VIEWPARSER_INCLUDE;
                bufferdata_pop_back(&parser->current_tag->result_content);

                if (!__viewparser_include_create(parser))
                    return 0;

                context->symbol = VIEWPARSER_FIGASTOPEN;

                break;
            }
            case '%':
            {
                if (context->symbol != VIEWPARSER_FIGFIGOPEN) {
                    bufferdata_push(&parser->current_tag->result_content, ch);
                    break;
                }

                context->stage = VIEWPARSER_BRANCH;
                context->symbol = VIEWPARSER_FIGPERCENTOPEN;

                bufferdata_pop_back(&parser->current_tag->result_content);

                break;
            }
            default:
                context->symbol = VIEWPARSER_DEFAULT;
                bufferdata_push(&parser->current_tag->result_content, ch);
                break;
            }

            break;
        }
        case VIEWPARSER_VARIABLE:
        {
            switch (ch)
            {
            case '{':
            case '%':
            {
                return 0;

                break;
            }
            case '}':
            {
                if (context->symbol == VIEWPARSER_FIGFIGOPEN) {
                    context->symbol = VIEWPARSER_FIGFIGCLOSE;
                    context->stage = VIEWPARSER_TEXT;
                    bufferdata_push(&parser->current_tag->result_content, ch);
                    break;
                }
                if (context->symbol == VIEWPARSER_FIGFIGCLOSE) {
                    context->stage = VIEWPARSER_TEXT;

                    if (!__viewparser_variable_item_complete_name(parser))
                        return 0;

                    if (!__viewparser_replace_tag_on_value(parser))
                        return 0;

                    parser->current_tag = parser->current_tag->parent;

                    break;
                }

                if (context->symbol == VIEWPARSER_VARIABLE_DOT)
                    return 0;

                context->symbol = VIEWPARSER_FIGFIGCLOSE;

                break;
            }
            default: __viewparser_variable_write_body(parser, ch);
            }
            
            break;
        }
        case VIEWPARSER_INCLUDE:
        {
            switch (ch)
            {
            case '{':
            case '%':
            {
                return 0;

                break;
            }
            case '*':
            {
                if (context->symbol == VIEWPARSER_FIGASTCLOSE)
                    return 0;

                context->symbol = VIEWPARSER_FIGASTCLOSE;

                break;
            }
            case '}':
            {
                if (context->symbol != VIEWPARSER_FIGASTCLOSE)
                    return 0;

                context->stage = VIEWPARSER_TEXT;
                context->symbol = VIEWPARSER_FIGFIGCLOSE;
                context->pos++;

                if (!__viewparser_include_load_content(parser))
                    return 0;

                goto start;
                break;
            }
            default: __viewparser_include_write_body(parser, ch);
            }

            break;
        }
        case VIEWPARSER_BRANCH:
        {
            switch (ch)
            {
            case '{':
            case '*':
            case '%':
            case '}':
            {
                return 0;

                break;
            }
            default: __viewparser_branch_write_body(parser, ch);
            }

            break;
        }
        case VIEWPARSER_CONDITION:
        {
            switch (ch)
            {
            case '{':
            case '*':
            {
                return 0;

                break;
            }
            case '%':
            {
                context->symbol = VIEWPARSER_FIGPERCENTCLOSE;

                break;
            }
            case '}':
            {
                if (context->symbol != VIEWPARSER_FIGPERCENTCLOSE)
                    return 0;

                context->stage = VIEWPARSER_TEXT;

                break;
            }
            default: __viewparser_branch_write_body(parser, ch);
            }

            break;
        }
        case VIEWPARSER_CONDITION_EXPR:
        {
            switch (ch)
            {
            case '{':
            case '*':
            {
                return 0;

                break;
            }
            case '%':
            {
                context->symbol = VIEWPARSER_FIGPERCENTCLOSE;

                break;
            }
            case '}':
            {
                if (context->symbol != VIEWPARSER_FIGPERCENTCLOSE)
                    return 0;

                context->stage = VIEWPARSER_TEXT;

                if (!__viewparser_condition_item_complete_name(parser))
                    return 0;

                if (!__viewparser_replace_condition_on_value(parser))
                    return 0;

                context->symbol = VIEWPARSER_FIGFIGCLOSE;

                break;
            }
            default: __viewparser_variable_write_body(parser, ch);
            }
            
            break;
        }
        default:
            break;
        }
    }

    if (parser->current_context->parent != NULL) {
        parser->current_context = parser->current_context->parent;
        parser->current_tag = parser->current_tag->parent;
        goto start;
    }

    return 1;
}

int __viewparser_variable_create(viewparser_t* parser) {
    viewparser_variable_t* variable = malloc(sizeof * variable);
    if (variable == NULL) return 0;

    viewparser_tag_t* tag = parser->current_tag;

    variable->base.item = NULL;
    variable->base.last_item = NULL;
    variable->base.next = NULL;
    variable->base.child = NULL;
    variable->base.last_child = NULL;
    variable->base.parent = tag;
    bufferdata_init(&variable->base.result_content);
    variable->base.type = VIEWPARSER_TAGTYPE_VAR;

    if (tag->last_child != NULL) {
        variable->base.parent_text_offset = tag->last_child->parent_text_offset + tag->last_child->parent_text_size;
        variable->base.parent_text_size = bufferdata_writed(&tag->result_content) - variable->base.parent_text_offset;
    }
    else {
        variable->base.parent_text_offset = 0;
        variable->base.parent_text_size = bufferdata_writed(&tag->result_content);
    }

    if (tag->child == NULL)
        tag->child = (viewparser_tag_t*)variable;

    if (tag->last_child != NULL)
        tag->last_child->next = (viewparser_tag_t*)variable;

    tag->last_child = (viewparser_tag_t*)variable;

    parser->current_tag = (viewparser_tag_t*)variable;

    return 1;
}

void __viewparser_variable_free(viewparser_variable_t* variable) {
    if (variable == NULL) return;

    viewparser_variable_item_t* item = variable->base.item;
    while (item) {
        viewparser_variable_item_t* next = item->next;
        __viewparser_variable_item_free(item);
        item = next;
    }

    variable->base.item = NULL;
    variable->base.last_item = NULL;
    // variable->base.childs
    // variable->base.result_content

    free(variable);
}

viewparser_variable_item_t* __viewparser_variable_item_create() {
    viewparser_variable_item_t* item = malloc(sizeof * item);
    if (item == NULL) return NULL;

    item->index = NULL;
    item->last_index = NULL;
    item->next = NULL;
    item->name[0] = 0;
    item->name_length = 0;

    return item;
}

int __viewparser_variable_item_append(viewparser_variable_item_t* variable_item, viewparser_tag_t* variable) {
    if (variable == NULL) return 0;

    if (variable->item == NULL)
        variable->item = variable_item;

    if (variable->last_item != NULL)
        variable->last_item->next = variable_item;

    variable->last_item = variable_item;

    return 1;
}

void __viewparser_variable_item_free(viewparser_variable_item_t* item) {
    if (item == NULL) return;

    viewparser_variable_index_t* index = item->index;
    while (index) {
        viewparser_variable_index_t* next = index->next;
        free(index);
        index = next;
    }

    free(item);
}

int __viewparser_variable_item_complete_name(viewparser_t* parser) {
    while (1) {
        if (bufferdata_back(&parser->variable_buffer) != ' ')
            break;

        bufferdata_pop_back(&parser->variable_buffer);
    }

    bufferdata_complete(&parser->variable_buffer);
    const size_t variable_item_size = bufferdata_writed(&parser->variable_buffer);
    if (variable_item_size >= VIEWPARSER_VARIABLE_ITEM_NAME_SIZE)
        return 0;

    if (variable_item_size > 0) {
        viewparser_tag_t* variable = parser->current_tag;
        strcpy(variable->last_item->name, bufferdata_get(&parser->variable_buffer));
        variable->last_item->name_length = variable_item_size;
    }

    bufferdata_reset(&parser->variable_buffer);

    return 1;
}

int __viewparser_variable_item_complete_index(viewparser_t* parser) {
    while (1) {
        if (bufferdata_back(&parser->variable_buffer) != ' ')
            break;

        bufferdata_pop_back(&parser->variable_buffer);
    }

    bufferdata_complete(&parser->variable_buffer);
    const size_t variable_item_size = bufferdata_writed(&parser->variable_buffer);
    if (variable_item_size > 10)
        return 0;

    if (variable_item_size > 0) {
        const char* value = bufferdata_get(&parser->variable_buffer);

        if (strspn(value, "0123456789") != variable_item_size)
            return 0;

        viewparser_tag_t* variable = parser->current_tag;
        if (!__viewparser_variable_item_index_create(variable->last_item, atoi(value)))
            return 0;
    }

    bufferdata_reset(&parser->variable_buffer);

    return 1;
}

int __viewparser_replace_tag_on_value(viewparser_t* parser) {
    viewparser_tag_t* variable = parser->current_tag;
    viewparser_variable_item_t* item = variable->item;
    viewparser_tag_t* data = variable->data_parent;
    const jsontok_t* json_token = NULL;
    if (data == NULL)
        json_token = json_root(parser->document);

    while (item) {
        const jsontok_t* token = json_object_get(json_token, item->name);
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
            if (json_is_string(token)) {
                const char* variable_value = json_string(token);
                for (size_t i = 0; i < strlen(variable_value); i++) {
                    // bufferdata_push(&parser->buf, variable_value[i]);
                }
            }
        }
        else {
            json_token = token;
        }

        item = item->next;
    }

    bufferdata_reset(&parser->variable_buffer);

    return 1;
}

int __viewparser_variable_item_index_create(viewparser_variable_item_t* item, int value) {
    if (item == NULL) return 0;

    viewparser_variable_index_t* index = malloc(sizeof * index);
    if (index == NULL) return 0;

    index->next = NULL;
    index->value = value;

    if (item->index == NULL)
        item->index = index;

    if (item->last_index != NULL)
        item->last_index->next = index;

    item->last_index = index;

    return 1;
}

int __viewparser_variable_write_body(viewparser_t* parser, char ch) {
    
    viewparser_context_t* const context = parser->current_context;

    switch (ch)
    {
    case ' ':
    {
        if (context->symbol == VIEWPARSER_FIGFIGOPEN)
            break;
        if (context->symbol == VIEWPARSER_VARIABLE_DOT)
            break;
        if (context->symbol == VIEWPARSER_VARIABLE_BRACKETOPEN)
            break;

        bufferdata_push(&parser->variable_buffer, ch);

        context->symbol = VIEWPARSER_VARIABLE_SPACE;

        break;
    }
    case '[':
    {
        if (context->symbol == VIEWPARSER_DEFAULT) {
            if (!__viewparser_variable_item_complete_name(parser))
                return 0;
        }
        else if (context->symbol == VIEWPARSER_VARIABLE_BRACKETCLOSE) {
            if (!__viewparser_variable_item_complete_index(parser))
                return 0;
        }
        else
            return 0;

        context->symbol = VIEWPARSER_VARIABLE_BRACKETOPEN;

        break;
    }
    case ']':
    {
        if (context->symbol == VIEWPARSER_VARIABLE_BRACKETOPEN)
            return 0;

        if (!__viewparser_variable_item_complete_index(parser))
            return 0;

        context->symbol = VIEWPARSER_VARIABLE_BRACKETCLOSE;

        break;
    }
    case '.':
    {
        if (context->symbol == VIEWPARSER_FIGFIGOPEN)
            return 0;
        if (context->symbol == VIEWPARSER_FIGPERCENTOPEN)
            return 0;
        if (context->symbol == VIEWPARSER_FIGASTOPEN)
            return 0;
        if (context->symbol == VIEWPARSER_VARIABLE_DOT)
            return 0;
        if (context->symbol == VIEWPARSER_VARIABLE_BRACKETOPEN)
            return 0;

        context->symbol = VIEWPARSER_VARIABLE_DOT;

        if (!__viewparser_variable_item_complete_name(parser))
            return 0;

        viewparser_variable_item_t* variable_item = __viewparser_variable_item_create();
        if (variable_item == NULL)
            return 0;

        #pragma GCC diagnostic ignored "-Wanalyzer-malloc-leak"
        if (!__viewparser_variable_item_append(variable_item, parser->current_tag))
            return 0;

        break;
    }
    default:
        context->symbol = VIEWPARSER_DEFAULT;
        bufferdata_push(&parser->variable_buffer, ch);
        break;
    }

    return 1;
}

int __viewparser_tag_init(viewparser_tag_t* tag, int type, viewparser_tag_t* data_parent, viewparser_tag_t* parent) {
    tag->type = type;
    tag->parent_text_offset = 0;
    tag->parent_text_size = 0;
    bufferdata_init(&tag->result_content);
    tag->data_parent = data_parent;
    tag->parent = parent;
    tag->child = NULL;
    tag->last_child = NULL;
    tag->next = NULL;
    tag->item = NULL;
    tag->last_item = NULL;

    return 1;
}

viewparser_tag_t* __viewparser_root_tag_create() {
    viewparser_tag_t* tag = malloc(sizeof * tag);
    if (tag == NULL) return NULL;

    __viewparser_tag_init(tag, VIEWPARSER_TAGTYPE_INC, NULL, NULL);

    return tag;
}

int __viewparser_include_create(viewparser_t* parser) {
    viewparser_include_t* tag = malloc(sizeof * tag);
    if (tag == NULL) return 0;

    viewparser_tag_t* current_tag = parser->current_tag;

    __viewparser_tag_init((viewparser_tag_t*)tag, VIEWPARSER_TAGTYPE_INC, current_tag->data_parent, current_tag);

    tag->is_control_key = 0;
    tag->control_key_index = 0;

    if (current_tag->last_child != NULL) {
        tag->base.parent_text_offset = current_tag->last_child->parent_text_offset + current_tag->last_child->parent_text_size;
        tag->base.parent_text_size = bufferdata_writed(&current_tag->result_content) - tag->base.parent_text_offset;
    }
    else {
        tag->base.parent_text_offset = 0;
        tag->base.parent_text_size = bufferdata_writed(&current_tag->result_content);
    }

    if (current_tag->child == NULL)
        current_tag->child = (viewparser_tag_t*)tag;

    if (current_tag->last_child != NULL)
        current_tag->last_child->next = (viewparser_tag_t*)tag;

    current_tag->last_child = (viewparser_tag_t*)tag;

    parser->current_tag = (viewparser_tag_t*)tag;

    return 1;
}

int __viewparser_include_write_body(viewparser_t* parser, char ch) {
    switch (ch)
    {
    case ' ':
    {
        if (parser->current_context->symbol == VIEWPARSER_FIGASTOPEN)
            break;

        bufferdata_push(&parser->variable_buffer, ch);

        parser->current_context->symbol = VIEWPARSER_VARIABLE_SPACE;

        break;
    }
    default:
    {
        viewparser_include_t* tag = (viewparser_include_t*)parser->current_tag;
        if (tag->is_control_key == 0) {
            if (__word_include[tag->control_key_index] != ch)
                return 0;

            if (tag->control_key_index >= 6) {
                tag->is_control_key = 1;
                parser->current_context->symbol = VIEWPARSER_FIGASTOPEN; // escape spaces after include word
                break;
            }
            else
                tag->control_key_index++;
        }
        else {
            bufferdata_push(&parser->variable_buffer, ch);
        }

        parser->current_context->symbol = VIEWPARSER_DEFAULT;

        break;
    }
    }

    return 1;
}

int __viewparser_include_load_content(viewparser_t* parser) {
    while (1) {
        if (bufferdata_back(&parser->variable_buffer) != ' ')
            break;

        bufferdata_pop_back(&parser->variable_buffer);
    }

    bufferdata_complete(&parser->variable_buffer);
    const size_t path_size = bufferdata_writed(&parser->variable_buffer);
    if (path_size >= 4096)
        return 0;

    if (path_size > 0) {
        viewparser_context_t* context = __viewparser_context_create();
        if (context == NULL) return 0;

        context->parent = parser->current_context;

        if (parser->current_context->child == NULL)
            parser->current_context->child = context;

        if (parser->current_context->last_child != NULL)
            parser->current_context->last_child->next = context;

        parser->current_context->last_child = context;
        parser->current_context = context;

        // parser->current_tag = parser->current_tag->last_child;

        const char* path = bufferdata_get(&parser->variable_buffer);

        if (!__viewparser_readfile(parser->current_context, parser->storage_name, path)) {
            log_error("View file not found %s", path);
            return 0;
        }
    }

    bufferdata_reset(&parser->variable_buffer);

    return 1;
}

int __viewparser_branch_write_body(viewparser_t* parser, char ch) {
    switch (ch)
    {
    case ' ':
    {
        if (parser->current_context->symbol == VIEWPARSER_FIGPERCENTOPEN)
            break;

        if (parser->current_context->symbol == VIEWPARSER_DEFAULT) {
            bufferdata_complete(&parser->variable_buffer);
            const size_t tag_name_size = bufferdata_writed(&parser->variable_buffer);
            const char* tag_name = bufferdata_get(&parser->variable_buffer);

            for (size_t i = 0; i < tag_name_size; i++) {
                const int complete = i + 1 == tag_name_size;

                if (tag_name_size == 2 && __word_if[i] == tag_name[i]) {
                    if (complete && !__viewparser_condition_if_create(parser))
                        return 0;

                    parser->current_context->stage = VIEWPARSER_CONDITION_EXPR;
                }
                else if (tag_name_size == 6 && __word_elseif[i] == tag_name[i]) {
                    if (complete && !__viewparser_condition_elseif_create(parser))
                        return 0;

                    parser->current_context->stage = VIEWPARSER_CONDITION_EXPR;
                }
                else if (tag_name_size == 4 && __word_else[i] == tag_name[i]) {
                    if (complete && !__viewparser_condition_else_create(parser))
                        return 0;

                    parser->current_context->stage = VIEWPARSER_CONDITION_EXPR;
                }
                else if (tag_name_size == 5 && __word_endif[i] == tag_name[i]) {
                    parser->current_tag = parser->current_tag->parent->parent;
                    parser->current_context->stage = VIEWPARSER_CONDITION;
                    break;
                }
                else if (tag_name_size == 3 && __word_for[i] == tag_name[i]) {
                    // if (complete && !__viewparser_loop_create(parser))
                    //     return 0;
                }
                else if (tag_name_size == 6 && __word_endfor[i] == tag_name[i]) {
                    // if (complete && !__viewparser_loop_create(parser))
                    //     return 0;
                }
                else if (complete)
                    return 0;
            }

            bufferdata_reset(&parser->variable_buffer);
            break;
        }

        bufferdata_push(&parser->variable_buffer, ch);

        parser->current_context->symbol = VIEWPARSER_VARIABLE_SPACE;

        break;
    }
    default:
    {
        bufferdata_push(&parser->variable_buffer, ch);

        parser->current_context->symbol = VIEWPARSER_DEFAULT;

        break;
    }
    }

    return 1;
}

int __viewparser_condition_if_create(viewparser_t* parser) {
    viewparser_tag_t* tag_cond = malloc(sizeof * tag_cond);
    if (tag_cond == NULL) return 0;

    viewparser_tag_t* current_tag = parser->current_tag;

    __viewparser_tag_init(tag_cond, VIEWPARSER_TAGTYPE_COND, current_tag->data_parent, current_tag);

    if (current_tag->last_child != NULL) {
        tag_cond->parent_text_offset = current_tag->last_child->parent_text_offset + current_tag->last_child->parent_text_size;
        tag_cond->parent_text_size = bufferdata_writed(&current_tag->result_content) - tag_cond->parent_text_offset;
    }
    else {
        tag_cond->parent_text_offset = 0;
        tag_cond->parent_text_size = bufferdata_writed(&current_tag->result_content);
    }

    viewparser_condition_item_t* tag_if = malloc(sizeof * tag_if);
    if (tag_if == NULL) {
        free(tag_cond);
        return 0;
    }

    __viewparser_tag_init((viewparser_tag_t*)tag_if, VIEWPARSER_TAGTYPE_COND_IF, current_tag->data_parent, tag_cond);
    tag_if->result = 0;
    tag_if->reverse = 0;
    tag_if->always_true = 0;

    viewparser_variable_item_t* variable_item = __viewparser_variable_item_create();
    if (variable_item == NULL) {
        free(tag_cond);
        free(tag_if);
        return 0;
    }

    if (!__viewparser_condition_item_append(variable_item, tag_if))
        return 0;

    if (tag_cond->child == NULL)
        tag_cond->child = (viewparser_tag_t*)tag_if;

    if (tag_cond->last_child != NULL)
        tag_cond->last_child->next = (viewparser_tag_t*)tag_if;

    tag_cond->last_child = (viewparser_tag_t*)tag_if;



    if (current_tag->child == NULL)
        current_tag->child = tag_cond;

    if (current_tag->last_child != NULL)
        current_tag->last_child->next = tag_cond;

    current_tag->last_child = tag_cond;

    parser->current_tag = (viewparser_tag_t*)tag_if;

    return 1;
}

int __viewparser_condition_item_append(viewparser_variable_item_t* variable_item, viewparser_condition_item_t* tag_if) {
    if (tag_if == NULL) return 0;

    if (tag_if->base.item == NULL)
        tag_if->base.item = variable_item;

    if (tag_if->base.last_item != NULL)
        tag_if->base.last_item->next = variable_item;

    tag_if->base.last_item = variable_item;

    return 1;
}

int __viewparser_condition_item_complete_name(viewparser_t* parser) {
    while (1) {
        if (bufferdata_back(&parser->variable_buffer) != ' ')
            break;

        bufferdata_pop_back(&parser->variable_buffer);
    }

    bufferdata_complete(&parser->variable_buffer);
    const size_t variable_item_size = bufferdata_writed(&parser->variable_buffer);
    if (variable_item_size >= VIEWPARSER_VARIABLE_ITEM_NAME_SIZE)
        return 0;

    if (variable_item_size > 0) {
        viewparser_tag_t* condition_item = parser->current_tag;
        strcpy(condition_item->last_item->name, bufferdata_get(&parser->variable_buffer));
        condition_item->last_item->name_length = variable_item_size;
    }

    bufferdata_reset(&parser->variable_buffer);

    return 1;
}

int __viewparser_replace_condition_on_value(viewparser_t* parser) {
    viewparser_tag_t* variable = parser->current_tag;
    viewparser_variable_item_t* item = variable->item;
    viewparser_tag_t* data = variable->data_parent;
    const jsontok_t* json_token = NULL;
    if (data == NULL)
        json_token = json_root(parser->document);

    while (item) {
        const jsontok_t* token = json_object_get(json_token, item->name);
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
            if (json_is_bool(token))
                ((viewparser_condition_item_t*)variable)->result = json_bool(token);
        }
        else {
            json_token = token;
        }

        item = item->next;
    }

    bufferdata_reset(&parser->variable_buffer);

    return 1;
}

int __viewparser_condition_elseif_create(viewparser_t* parser) {
    viewparser_tag_t* current_tag = parser->current_tag;
    if (current_tag->type != VIEWPARSER_TAGTYPE_COND_IF && current_tag->type != VIEWPARSER_TAGTYPE_COND_ELSEIF)
        return 0;

    viewparser_tag_t* tag_cond = parser->current_tag->parent;

    viewparser_condition_item_t* tag_elseif = malloc(sizeof * tag_elseif);
    if (tag_elseif == NULL)
        return 0;

    __viewparser_tag_init((viewparser_tag_t*)tag_elseif, VIEWPARSER_TAGTYPE_COND_ELSEIF, current_tag->data_parent, tag_cond);
    tag_elseif->result = 0;
    tag_elseif->reverse = 0;
    tag_elseif->always_true = 0;

    viewparser_variable_item_t* variable_item = __viewparser_variable_item_create();
    if (variable_item == NULL) {
        free(tag_cond);
        free(tag_elseif);
        return 0;
    }

    if (!__viewparser_condition_item_append(variable_item, tag_elseif))
        return 0;

    if (tag_cond->child == NULL)
        tag_cond->child = (viewparser_tag_t*)tag_elseif;

    if (tag_cond->last_child != NULL)
        tag_cond->last_child->next = (viewparser_tag_t*)tag_elseif;

    tag_cond->last_child = (viewparser_tag_t*)tag_elseif;


    parser->current_tag = (viewparser_tag_t*)tag_elseif;

    return 1;
}

int __viewparser_condition_else_create(viewparser_t* parser) {
    viewparser_tag_t* current_tag = parser->current_tag;
    if (current_tag->type != VIEWPARSER_TAGTYPE_COND_IF && current_tag->type != VIEWPARSER_TAGTYPE_COND_ELSEIF)
        return 0;

    viewparser_tag_t* tag_cond = parser->current_tag->parent;

    viewparser_condition_item_t* tag_else = malloc(sizeof * tag_else);
    if (tag_else == NULL)
        return 0;

    __viewparser_tag_init((viewparser_tag_t*)tag_else, VIEWPARSER_TAGTYPE_COND_ELSE, current_tag->data_parent, tag_cond);
    tag_else->result = 0;
    tag_else->reverse = 0;
    tag_else->always_true = 1;

    viewparser_variable_item_t* variable_item = __viewparser_variable_item_create();
    if (variable_item == NULL) {
        free(tag_cond);
        free(tag_else);
        return 0;
    }

    if (!__viewparser_condition_item_append(variable_item, tag_else))
        return 0;

    if (tag_cond->child == NULL)
        tag_cond->child = (viewparser_tag_t*)tag_else;

    if (tag_cond->last_child != NULL)
        tag_cond->last_child->next = (viewparser_tag_t*)tag_else;

    tag_cond->last_child = (viewparser_tag_t*)tag_else;


    parser->current_tag = (viewparser_tag_t*)tag_else;

    return 1;
}

void __viewparser_build_content(viewparser_t* parser) {
    viewparser_tag_t* child = parser->root_tag->child;
    if (child == NULL) {
        size_t size = bufferdata_writed(&parser->root_tag->result_content);
        const char* content = bufferdata_get(&parser->root_tag->result_content);

        for (size_t i = 0; i < size; i++)
            bufferdata_push(&parser->buf, content[i]);
    }

    __viewparser_build_content_recursive(parser, child);
}

void __viewparser_build_content_recursive(viewparser_t* parser, viewparser_tag_t* tag) {
    viewparser_tag_t* child = tag;
    while (child) {
        switch (child->type) {
        case VIEWPARSER_TAGTYPE_VAR:
        {
            viewparser_tag_t* parent = child->parent;

            size_t size = bufferdata_writed(&parent->result_content);
            const char* content = bufferdata_get(&parent->result_content);
            for (size_t i = child->parent_text_offset; i < child->parent_text_offset + child->parent_text_size && i < size; i++)
                bufferdata_push(&parser->buf, content[i]);

            content = __viewparser_get_tag_value(parser, child);
            if (content != NULL) {
                size = strlen(content);
                for (size_t i = 0; i < size; i++)
                    bufferdata_push(&parser->buf, content[i]);
            }

            if (child->next == NULL) {
                size = bufferdata_writed(&parent->result_content);
                content = bufferdata_get(&parent->result_content);
                for (size_t i = child->parent_text_offset + child->parent_text_size; i < size; i++)
                    bufferdata_push(&parser->buf, content[i]);

                // child = child->parent;

                if (child == tag) return;
            }

            child = child->next;

            break;
        }
        case VIEWPARSER_TAGTYPE_COND:
        {
            // viewparser_tag_t* parent = child->parent;

            // const size_t size = bufferdata_writed(&parent->result_content);
            // const char* content = bufferdata_get(&parent->result_content);
            // for (size_t i = child->parent_text_offset; i < child->parent_text_offset + child->parent_text_size && i < size; i++)
            //     bufferdata_push(&parser->buf, content[i]);

            __viewparser_build_content_recursive(parser, child->child);

            // if (child->next == NULL) {
            //     // const size_t size = bufferdata_writed(&parent->result_content);
            //     // const char* content = bufferdata_get(&parent->result_content);
            //     // for (size_t i = child->parent_text_offset + child->parent_text_size; i < size; i++)
            //     //     bufferdata_push(&parser->buf, content[i]);

            //     // child = child->parent;

            //     if (child == tag) return;
            // }

            child = child->next;

            break;
        }
        case VIEWPARSER_TAGTYPE_COND_IF:
        case VIEWPARSER_TAGTYPE_COND_ELSEIF:
        case VIEWPARSER_TAGTYPE_COND_ELSE:
        {
            viewparser_tag_t* parent = child->parent;
            viewparser_condition_item_t* tag = (viewparser_condition_item_t*)child;

            const int istrue = tag->always_true || (tag->result && !tag->reverse) || (!tag->result && tag->reverse);

            if (!istrue) {
                if (child->next != NULL) {
                    child = child->next;
                    break;
                }
            }

            size_t size = bufferdata_writed(&parent->parent->result_content);
            const char* content = bufferdata_get(&parent->parent->result_content);
            for (size_t i = parent->parent_text_offset; i < parent->parent_text_offset + parent->parent_text_size; i++)
                bufferdata_push(&parser->buf, content[i]);

            if (istrue) {
                if (child->child != NULL) {
                    __viewparser_build_content_recursive(parser, child->child);
                }
                else {
                    const size_t size = bufferdata_writed(&child->result_content);
                    const char* content = bufferdata_get(&child->result_content);
                    for (size_t i = 0; i < size; i++)
                        bufferdata_push(&parser->buf, content[i]);
                }
            }

            if (parent->next == NULL) {
                size = bufferdata_writed(&parent->parent->result_content);
                content = bufferdata_get(&parent->parent->result_content);
                for (size_t i = parent->parent_text_offset + parent->parent_text_size; i < size; i++)
                    bufferdata_push(&parser->buf, content[i]);
            }

            if (istrue) return;

            child = child->next;

            break;
        }
        case VIEWPARSER_TAGTYPE_LOOP:
        {
            break;
        }
        case VIEWPARSER_TAGTYPE_INC:
        {
            viewparser_tag_t* parent = child->parent;

            if (parent != NULL) {
                const char* content = bufferdata_get(&parent->result_content);
                for (size_t i = child->parent_text_offset; i < child->parent_text_offset + child->parent_text_size; i++)
                    bufferdata_push(&parser->buf, content[i]);
            }

            if (child->child != NULL) {
                __viewparser_build_content_recursive(parser, child->child);
            }
            else {
                const size_t size = bufferdata_writed(&child->result_content);
                const char* content = bufferdata_get(&child->result_content);
                for (size_t i = 0; i < size; i++)
                    bufferdata_push(&parser->buf, content[i]);
            }

            if (child->next == NULL && parent != NULL) {
                const size_t size = bufferdata_writed(&parent->result_content);
                const char* content = bufferdata_get(&parent->result_content);
                for (size_t i = child->parent_text_offset + child->parent_text_size; i < size; i++)
                    bufferdata_push(&parser->buf, content[i]);
            }

            child = child->next;

            break;
        }
        }
    }
}

const char* __viewparser_get_tag_value(viewparser_t* parser, viewparser_tag_t* tag) {
    viewparser_variable_item_t* item = tag->item;
    viewparser_tag_t* data = tag->data_parent;
    const jsontok_t* json_token = NULL;
    if (data == NULL)
        json_token = json_root(parser->document);

    while (item) {
        const jsontok_t* token = json_object_get(json_token, item->name);
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
            if (json_is_string(token)) {
                return json_string(token);
            }
        }
        else {
            json_token = token;
        }

        item = item->next;
    }

    return NULL;
}
