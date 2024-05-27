#include "storage.h"
#include "log.h"
#include "viewparser.h"

static viewparser_context_t* __viewparser_context_create();
static int __viewparser_readfile(viewparser_context_t* context, const char* storage_name,const char* path);
static int __viewparser_parse_content(viewparser_t* parser);
static int __viewparser_variable_create(viewparser_t* parser);
static void __viewparser_variable_free(viewparser_variable_t* variable);
static int __viewparser_variable_item_create(viewparser_variable_t* variable);
static void __viewparser_variable_item_free(viewparser_variable_item_t* item);
static int __viewparser_variable_item_complete_name(viewparser_t* parser);
static int __viewparser_variable_item_complete_index(viewparser_t* parser);
static int __viewparser_replace_tag_on_value(viewparser_t* parser);
static int __viewparser_variable_item_index_create(viewparser_variable_item_t* item, int value);
static int __viewparser_variable_write_body(viewparser_t* parser, char ch);

static viewparser_tag_t* __viewparser_root_tag_create(const jsondoc_t* document);
static int __viewparser_include_create(viewparser_t* parser);
static int __viewparser_include_write_body(viewparser_t* parser, char ch);

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

    parser->root_tag = __viewparser_root_tag_create(parser->document);
    if (parser->root_tag == NULL) return 0;

    parser->current_tag = parser->root_tag;
    parser->current_context = parser->context;

    if (!__viewparser_readfile(parser->current_context, parser->storage_name, parser->path)) {
        log_error("root file not found %s", parser->path);
        return 0;
    }

    __viewparser_parse_content(parser);

    return 1;
}

char* viewparser_get_content(viewparser_t* parser) {
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
    context->pos_start = 0;
    context->pos = 0;
    context->buffer = NULL;
    context->parent = NULL;
    context->childs = NULL;

    return context;
}

int __viewparser_readfile(viewparser_context_t* context, const char* storage_name,const char* path) {
    file_t file = storage_file_get(storage_name, path);
    if (!file.ok)
        return 0;

    context->buffer = file.content(&file);
    context->bytes_readed = file.size;

    file.close(&file);

    if (context->buffer == NULL)
        return 0;

    return 1;
}

int __viewparser_parse_content(viewparser_t* parser) {
    viewparser_context_t* context = parser->context;
    for (context->pos = context->pos_start; context->pos < context->bytes_readed; context->pos++) {
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
                    if (!__viewparser_variable_create(parser))
                        return 0;

                    if (!__viewparser_variable_item_create((viewparser_variable_t*)parser->current_tag->last_child))
                        return 0;

                    bufferdata_pop_back(&parser->buf);
                    break;
                }

                context->symbol = VIEWPARSER_FIGFIGOPEN;
                bufferdata_push(&parser->buf, ch);

                break;
            }
            case '*':
            {
                if (context->symbol != VIEWPARSER_FIGASTOPEN) {
                    bufferdata_push(&parser->buf, ch);
                    break;
                }

                context->stage = VIEWPARSER_INCLUDE;
                if (!__viewparser_include_create(parser))
                    return 0;

                // if (!__viewparser_variable_item_create((viewparser_variable_t*)parser->current_tag->last_child))
                //     return 0;

                bufferdata_pop_back(&parser->buf);

                context->symbol = VIEWPARSER_FIGASTOPEN;

                break;
            }
            default:
                context->symbol = VIEWPARSER_DEFAULT;
                bufferdata_push(&parser->buf, ch);
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
                    bufferdata_push(&parser->buf, ch);
                    break;
                }
                if (context->symbol == VIEWPARSER_FIGFIGCLOSE) {
                    context->stage = VIEWPARSER_TEXT;

                    if (!__viewparser_variable_item_complete_name(parser))
                        return 0;

                    if (!__viewparser_replace_tag_on_value(parser))
                        return 0;

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
                if (context->symbol == VIEWPARSER_FIGASTCLOSE) {
                    context->stage = VIEWPARSER_TEXT;

                    // if (!__viewparser_variable_item_complete_name(parser))
                    //     return 0;

                    // if (!__viewparser_replace_tag_on_value(parser))
                    //     return 0;

                    break;
                }

                bufferdata_push(&parser->buf, ch);

                context->symbol = VIEWPARSER_FIGFIGCLOSE;

                break;
            }
            default: __viewparser_include_write_body(parser, ch);
            }

            break;
        }
        default:
            break;
        }
    }

    return 1;
}

int __viewparser_variable_create(viewparser_t* parser) {
    viewparser_variable_t* variable = malloc(sizeof * variable);
    if (variable == NULL) return 0;

    viewparser_tag_t* tag = parser->current_tag;

    variable->base.json_token = parser->current_tag->json_token;
    variable->item = NULL;
    variable->last_item = NULL;
    variable->base.next = NULL;
    variable->base.child = NULL;
    variable->base.last_child = NULL;
    variable->base.parent = tag;
    variable->base.parent_position = 0;
    bufferdata_init(&variable->base.result_content);
    variable->base.type = VIEWPARSER_TAGTYPE_VAR;
    
    if (tag->child == NULL)
        tag->child = (viewparser_tag_t*)variable;

    if (tag->last_child != NULL)
        tag->last_child->next = (viewparser_tag_t*)variable;

    tag->last_child = (viewparser_tag_t*)variable;

    return 1;
}

void __viewparser_variable_free(viewparser_variable_t* variable) {
    if (variable == NULL) return;

    viewparser_variable_item_t* item = variable->item;
    while (item) {
        viewparser_variable_item_t* next = item->next;
        __viewparser_variable_item_free(item);
        item = next;
    }

    variable->item = NULL;
    variable->last_item = NULL;
    // variable->base.childs
    // variable->base.result_content

    free(variable);
}

int __viewparser_variable_item_create(viewparser_variable_t* variable) {
    if (variable == NULL) return 0;

    viewparser_variable_item_t* item = malloc(sizeof * item);
    if (item == NULL) return 0;

    item->index = NULL;
    item->last_index = NULL;
    item->next = NULL;
    item->name[0] = 0;
    item->name_length = 0;

    if (variable->item == NULL)
        variable->item = item;

    if (variable->last_item != NULL)
        variable->last_item->next = item;

    variable->last_item = item;

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
        viewparser_variable_t* variable = (viewparser_variable_t*)parser->current_tag->last_child;
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

        viewparser_variable_t* variable = (viewparser_variable_t*)parser->current_tag->last_child;
        if (!__viewparser_variable_item_index_create(variable->last_item, atoi(value)))
            return 0;
    }

    bufferdata_reset(&parser->variable_buffer);

    return 1;
}

int __viewparser_replace_tag_on_value(viewparser_t* parser) {
    viewparser_variable_t* variable = (viewparser_variable_t*)parser->current_tag->last_child;
    viewparser_variable_item_t* item = variable->item;
    const jsontok_t* json_token = variable->base.json_token;
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
                    bufferdata_push(&parser->buf, variable_value[i]);
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
    switch (ch)
    {
    case ' ':
    {
        if (parser->current_context->symbol == VIEWPARSER_FIGFIGOPEN)
            break;
        if (parser->current_context->symbol == VIEWPARSER_VARIABLE_DOT)
            break;
        if (parser->current_context->symbol == VIEWPARSER_VARIABLE_BRACKETOPEN)
            break;

        bufferdata_push(&parser->variable_buffer, ch);

        parser->current_context->symbol = VIEWPARSER_VARIABLE_SPACE;

        break;
    }
    case '[':
    {
        if (parser->current_context->symbol == VIEWPARSER_DEFAULT) {
            if (!__viewparser_variable_item_complete_name(parser))
                return 0;
        }
        else if (parser->current_context->symbol == VIEWPARSER_VARIABLE_BRACKETCLOSE) {
            if (!__viewparser_variable_item_complete_index(parser))
                return 0;
        }
        else
            return 0;

        parser->current_context->symbol = VIEWPARSER_VARIABLE_BRACKETOPEN;

        break;
    }
    case ']':
    {
        if (parser->current_context->symbol == VIEWPARSER_VARIABLE_BRACKETOPEN)
            return 0;

        if (!__viewparser_variable_item_complete_index(parser))
            return 0;

        parser->current_context->symbol = VIEWPARSER_VARIABLE_BRACKETCLOSE;

        break;
    }
    case '.':
    {
        if (parser->current_context->symbol == VIEWPARSER_FIGFIGOPEN)
            return 0;
        if (parser->current_context->symbol == VIEWPARSER_FIGPERCENTOPEN)
            return 0;
        if (parser->current_context->symbol == VIEWPARSER_FIGASTOPEN)
            return 0;
        if (parser->current_context->symbol == VIEWPARSER_VARIABLE_DOT)
            return 0;
        if (parser->current_context->symbol == VIEWPARSER_VARIABLE_BRACKETOPEN)
            return 0;

        parser->current_context->symbol = VIEWPARSER_VARIABLE_DOT;

        if (!__viewparser_variable_item_complete_name(parser))
            return 0;

        if (!__viewparser_variable_item_create((viewparser_variable_t*)parser->current_tag->last_child))
            return 0;

        break;
    }
    default:
        parser->current_context->symbol = VIEWPARSER_DEFAULT;
        bufferdata_push(&parser->variable_buffer, ch);
        break;
    }

    return 1;
}

viewparser_tag_t* __viewparser_root_tag_create(const jsondoc_t* document) {
    viewparser_tag_t* tag = malloc(sizeof * tag);
    if (tag == NULL) return NULL;

    tag->type = VIEWPARSER_TAGTYPE_INC;
    tag->parent_position = 0;
    bufferdata_init(&tag->result_content);
    tag->json_token = json_root(document);
    tag->parent = NULL;
    tag->child = NULL;
    tag->last_child = NULL;
    tag->next = NULL;

    return tag;
}

int __viewparser_include_create(viewparser_t* parser) {
    viewparser_tag_t* tag = malloc(sizeof * tag);
    if (tag == NULL) return 0;

    viewparser_tag_t* current_tag = parser->current_tag;

    tag->type = VIEWPARSER_TAGTYPE_INC;
    tag->parent_position = 0;
    bufferdata_init(&tag->result_content);
    tag->json_token = parser->current_tag->json_token;
    tag->parent = current_tag;
    tag->child = NULL;
    tag->last_child = NULL;
    tag->next = NULL;

    if (current_tag->child == NULL)
        current_tag->child = (viewparser_tag_t*)tag;

    if (current_tag->last_child != NULL)
        current_tag->last_child->next = (viewparser_tag_t*)tag;

    current_tag->last_child = (viewparser_tag_t*)tag;

    return 1;
}

int __viewparser_include_write_body(viewparser_t* parser, char ch) {
    switch (ch)
    {
    case ' ':
    {
        if (parser->current_context->symbol == VIEWPARSER_FIGFIGOPEN)
            break;
        if (parser->current_context->symbol == VIEWPARSER_VARIABLE_DOT)
            break;
        if (parser->current_context->symbol == VIEWPARSER_VARIABLE_BRACKETOPEN)
            break;

        bufferdata_push(&parser->variable_buffer, ch);

        parser->current_context->symbol = VIEWPARSER_VARIABLE_SPACE;

        break;
    }
    case '[':
    {
        if (parser->current_context->symbol == VIEWPARSER_DEFAULT) {
            if (!__viewparser_variable_item_complete_name(parser))
                return 0;
        }
        else if (parser->current_context->symbol == VIEWPARSER_VARIABLE_BRACKETCLOSE) {
            if (!__viewparser_variable_item_complete_index(parser))
                return 0;
        }
        else
            return 0;

        parser->current_context->symbol = VIEWPARSER_VARIABLE_BRACKETOPEN;

        break;
    }
    case ']':
    {
        if (parser->current_context->symbol == VIEWPARSER_VARIABLE_BRACKETOPEN)
            return 0;

        if (!__viewparser_variable_item_complete_index(parser))
            return 0;

        parser->current_context->symbol = VIEWPARSER_VARIABLE_BRACKETCLOSE;

        break;
    }
    case '.':
    {
        if (parser->current_context->symbol == VIEWPARSER_FIGFIGOPEN)
            return 0;
        if (parser->current_context->symbol == VIEWPARSER_FIGPERCENTOPEN)
            return 0;
        if (parser->current_context->symbol == VIEWPARSER_FIGASTOPEN)
            return 0;
        if (parser->current_context->symbol == VIEWPARSER_VARIABLE_DOT)
            return 0;
        if (parser->current_context->symbol == VIEWPARSER_VARIABLE_BRACKETOPEN)
            return 0;

        parser->current_context->symbol = VIEWPARSER_VARIABLE_DOT;

        if (!__viewparser_variable_item_complete_name(parser))
            return 0;

        if (!__viewparser_variable_item_create((viewparser_variable_t*)parser->current_tag->last_child))
            return 0;

        break;
    }
    default:
        parser->current_context->symbol = VIEWPARSER_DEFAULT;
        bufferdata_push(&parser->variable_buffer, ch);
        break;
    }

    return 1;
}
