#include <stdlib.h>

#include "storage.h"
#include "log.h"
#include "viewparser.h"

#define VIEWPARSER_MAX_DEPTH 100

static viewparser_context_t* __viewparser_context_create();
static void __viewparser_context_free(viewparser_context_t* context);
static int __viewparser_readfile(viewparser_context_t* context, const char* storage_name,const char* path);
static int __viewparser_parse_content(viewparser_t* parser);
static int __viewparser_variable_create(viewparser_t* parser);
static viewparser_variable_item_t* __viewparser_variable_item_create();
static void __viewparser_variable_item_append(viewparser_variable_item_t* variable_item, viewparser_tag_t* variable);
static int __viewparser_variable_item_complete_name(viewparser_t* parser);
static int __viewparser_variable_item_complete_index(viewparser_t* parser);
static int __viewparser_variable_item_index_create(viewparser_variable_item_t* item, int value);
static int __viewparser_variable_write_body(viewparser_t* parser, char ch);
static void __viewparser_variable_rtrim(viewparser_t* parser);
static void __viewparser_tag_append_child_or_sibling(viewparser_tag_t* parent, viewparser_tag_t* child);
static void __viewparser_tag_set_parent_text_params(viewparser_tag_t* tag, viewparser_tag_t* parent);
static viewparser_tag_t* __viewparser_get_dataparent(viewparser_tag_t* tag);

static void __viewparser_tag_init(viewparser_tag_t* tag, viewparser_tagtype_e type, viewparser_tag_t* data_parent, viewparser_tag_t* parent);
static viewparser_tag_t* __viewparser_root_tag_create();
static int __viewparser_include_create(viewparser_t* parser);
static int __viewparser_include_write_body(viewparser_t* parser, char ch);
static int __viewparser_include_load_content(viewparser_t* parser);

static int __viewparser_branch_write_body(viewparser_t* parser);
static int __viewparser_condition_if_create(viewparser_t* parser);
static int __viewparser_condition_elseif_create(viewparser_t* parser);
static int __viewparser_condition_else_create(viewparser_t* parser);
static void __viewparser_condition_item_append(viewparser_variable_item_t* variable_item, viewparser_condition_item_t* tag);
static int __viewparser_condition_item_complete_name(viewparser_t* parser);

static int __viewparser_loop_create(viewparser_t* parser);
static int __viewparser_loop_write_item(viewparser_t* parser, char ch);
static int __viewparser_loop_write_index(viewparser_t* parser, char ch);
static int __viewparser_loop_write_in(viewparser_t* parser, char ch);

static char __word_include[8] = {'i', 'n', 'c', 'l', 'u', 'd', 'e', 0};
static char __word_if[3] = {'i', 'f', 0};
static char __word_elseif[7] = {'e', 'l', 's', 'e', 'i', 'f', 0};
static char __word_else[5] = {'e', 'l', 's', 'e', 0};
static char __word_endif[6] = {'e', 'n', 'd', 'i', 'f', 0};
static char __word_for[4] = {'f', 'o', 'r', 0};
static char __word_endfor[7] = {'e', 'n', 'd', 'f', 'o', 'r', 0};

/**
 * Initialize a view parser.
 *
 * @param storage_name The name of the storage to use.
 * @param path The path to the view file.
 * @return A pointer to the initialized view parser.
 */
viewparser_t* viewparser_init(const char* restrict storage_name, const char* restrict path) {
    viewparser_t* parser = malloc(sizeof * parser);
    if (parser == NULL) {
        log_error("viewparser_init: failed to allocate memory for view parser");
        return NULL;
    }

    parser->storage_name = storage_name;
    parser->path = path;
    parser->context = NULL;
    parser->current_context = NULL;
    parser->root_tag = NULL;
    parser->current_tag = NULL;
    parser->level = 0;

    bufferdata_init(&parser->variable_buffer);

    return parser;
}


/**
 * Initialize and run a view parser.
 *
 * @param parser The view parser to initialize and run.
 * @return Non-zero if the view parser was successfully initialized and
 *         ran; zero otherwise.
 */
int viewparser_run(viewparser_t* parser) {
    parser->context = __viewparser_context_create();
    if (parser->context == NULL) {
        log_error("viewparser_run: failed to initialize view parser");
        return 0;
    }

    parser->root_tag = __viewparser_root_tag_create();
    if (parser->root_tag == NULL) {
        log_error("viewparser_run: failed to create root tag");
        return 0;
    }

    parser->current_tag = parser->root_tag;
    parser->current_context = parser->context;

    if (!__viewparser_readfile(parser->current_context, parser->storage_name, parser->path)) {
        log_error("viewparser_run: failed to read view file %s", parser->path);
        return 0;
    }

    if (!__viewparser_parse_content(parser))
        return 0;

    return 1;
}

/**
 * Moves the root tag of a view parser to NULL and returns the original root tag.
 *
 * @param parser Pointer to the view parser.
 * @return Pointer to the original root tag.
 */
viewparser_tag_t* viewparser_move_root_tag(viewparser_t* parser) {
    viewparser_tag_t* root_tag = parser->root_tag;

    parser->root_tag = NULL;
    parser->current_tag = NULL;

    return root_tag;
}

/**
 * Frees the memory used by a view parser.
 *
 * @param parser Pointer to the view parser.
 * @return void
 */
void viewparser_free(viewparser_t* parser) {
    if (parser == NULL) return;

    bufferdata_clear(&parser->variable_buffer);

    __viewparser_context_free(parser->context);

    if (parser->root_tag != NULL)
        parser->root_tag->free(parser->root_tag);

    free(parser);
}

/**
 * Creates a new view parser context.
 *
 * @return A pointer to the newly created view parser context, or NULL if
 *         memory allocation fails.
 */
viewparser_context_t* __viewparser_context_create(void) {
    viewparser_context_t* context = malloc(sizeof * context);
    if (context == NULL) return NULL;

    context->stage = VIEWPARSER_TEXT;
    context->symbol = VIEWPARSER_DEFAULT;
    context->bytes_readed = 0;
    context->pos = 0;
    context->buffer = NULL;
    context->parent = NULL;
    context->child = NULL;
    context->last_child = NULL;
    context->next = NULL;

    return context;
}

/**
 * Frees the memory used by a view parser context.
 *
 * @param context Pointer to the view parser context.
 * @return void
 */
void __viewparser_context_free(viewparser_context_t* context) {
    if (context == NULL) return;

    if (context->buffer != NULL)
        free(context->buffer);

    __viewparser_context_free(context->child);
    __viewparser_context_free(context->next);

    free(context);
}

/**
 * Reads the content of a file into a view parser context.
 *
 * @param context Pointer to the view parser context.
 * @param storage_name The name of the storage to use.
 * @param path The path to the file.
 * @return Non-zero if the file was successfully read; zero otherwise.
 */
int __viewparser_readfile(viewparser_context_t* context, const char* restrict storage_name, const char* restrict path) {
    file_t file = storage_file_get(storage_name, path);
    if (!file.ok)
        return 0;

    context->buffer = file.content(&file);
    context->bytes_readed = file.size;

    file.close(&file);

    return 1;
}

/**
 * Parses the content of a view file and constructs a view parser tree.
 *
 * @param parser Pointer to the view parser.
 * @return Non-zero if the parsing was successful; zero otherwise.
 */
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

                    __viewparser_variable_item_append(variable_item, parser->current_tag);
                    
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
            {
                return 0;
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

                if (!__viewparser_branch_write_body(parser))
                    return 0;

                context->stage = VIEWPARSER_TEXT;

                break;
            }
            case ' ':
            {
                if (!__viewparser_branch_write_body(parser))
                    return 0;

                break;
            }
            default:
            {
                bufferdata_push(&parser->variable_buffer, ch);
                parser->current_context->symbol = VIEWPARSER_DEFAULT;

                break;
            }
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

                context->symbol = VIEWPARSER_FIGFIGCLOSE;

                break;
            }
            default: __viewparser_variable_write_body(parser, ch);
            }
            
            break;
        }
        case VIEWPARSER_LOOP_ITEM:
        {
            switch (ch)
            {
            case '{':
            case '*':
            case '%':
            case '}':
            {
                return 0;
            }
            default: __viewparser_loop_write_item(parser, ch);
            }
            
            break;
        }
        case VIEWPARSER_LOOP_INDEX:
        {
            switch (ch)
            {
            case '{':
            case '*':
            case '%':
            case '}':
            {
                return 0;
            }
            default: __viewparser_loop_write_index(parser, ch);
            }
            
            break;
        }
        case VIEWPARSER_LOOP_IN:
        {
            switch (ch)
            {
            case '{':
            case '*':
            case '%':
            case '}':
            {
                return 0;
            }
            default: __viewparser_loop_write_in(parser, ch);
            }
            
            break;
        }
        case VIEWPARSER_LOOP_VAR:
        {
            switch (ch)
            {
            case '{':
            case '*':
            {
                return 0;
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

                if (!__viewparser_variable_item_complete_name(parser))
                    return 0;

                viewparser_loop_t* tag_for = (viewparser_loop_t*)parser->current_tag;
                if (tag_for->key_name[0] == 0) {
                    if (strcmp(tag_for->element_name, "index") == 0)
                        return 0;

                    strcpy(tag_for->key_name, "index");
                }

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
        parser->level--;

        goto start;
    }

    return 1;
}

/**
 * Creates a new variable tag in the view parser.
 *
 * @param parser The view parser object.
 * @return 1 if the variable tag was successfully created, 0 otherwise.
 */
int __viewparser_variable_create(viewparser_t* parser) {
    viewparser_tag_t* variable = malloc(sizeof * variable);
    if (variable == NULL) {
        log_error("__viewparser_variable_create: malloc failed\n");
        return 0;
    }

    viewparser_tag_t* current_tag = parser->current_tag;
    viewparser_tag_t* data_parent = __viewparser_get_dataparent(current_tag);

    __viewparser_tag_init(variable, VIEWPARSER_TAGTYPE_VAR, data_parent, current_tag);
    __viewparser_tag_set_parent_text_params(variable, current_tag);
    __viewparser_tag_append_child_or_sibling(current_tag, variable);

    parser->current_tag = variable;

    return 1;
}

/**
 * Creates a new variable item.
 *
 * @return A pointer to the created variable item, or NULL if it failed.
 */
viewparser_variable_item_t* __viewparser_variable_item_create(void) {
    viewparser_variable_item_t* item = malloc(sizeof * item);
    if (item == NULL) {
        log_error("__viewparser_variable_item_create: malloc failed\n");
        return NULL;
    }

    item->index = NULL;
    item->last_index = NULL;
    item->next = NULL;
    item->name[0] = 0;
    item->name_length = 0;

    return item;
}

/**
 * Appends a new variable item to a variable tag.
 *
 * @param variable_item The variable item to append.
 * @param variable The variable tag to append the variable item to.
 * @return void
 */
void __viewparser_variable_item_append(viewparser_variable_item_t* variable_item, viewparser_tag_t* variable) {
    if (variable->item == NULL)
        variable->item = variable_item;

    if (variable->last_item != NULL)
        variable->last_item->next = variable_item;

    variable->last_item = variable_item;
}

/**
 * Completes the name of a variable item.
 *
 * @param parser The view parser.
 * @return 1 if the name was successfully completed, 0 otherwise.
 */
int __viewparser_variable_item_complete_name(viewparser_t* parser) {
    __viewparser_variable_rtrim(parser);

    bufferdata_complete(&parser->variable_buffer);
    const size_t variable_item_size = bufferdata_writed(&parser->variable_buffer);
    if (variable_item_size >= VIEWPARSER_VARIABLE_ITEM_NAME_SIZE) {
        log_error("__viewparser_variable_item_complete_name: name size is greater or equal to %zd", VIEWPARSER_VARIABLE_ITEM_NAME_SIZE);
        return 0;
    }

    if (variable_item_size > 0) {
        viewparser_tag_t* variable = parser->current_tag;
        strcpy(variable->last_item->name, bufferdata_get(&parser->variable_buffer));
        variable->last_item->name_length = variable_item_size;
    }

    bufferdata_reset(&parser->variable_buffer);

    return 1;
}

/**
 * Completes the index of a variable item.
 *
 * @param parser The view parser.
 * @return 1 if the index was successfully completed, 0 otherwise.
 */
int __viewparser_variable_item_complete_index(viewparser_t* parser) {
    __viewparser_variable_rtrim(parser);

    bufferdata_complete(&parser->variable_buffer);
    const size_t variable_item_size = bufferdata_writed(&parser->variable_buffer);
    if (variable_item_size > 10) {
        log_error("__viewparser_variable_item_complete_index: variable_item_size is greater than 10");
        return 0;
    }

    if (variable_item_size > 0) {
        const char* value = bufferdata_get(&parser->variable_buffer);
        int index = 0;

        for (size_t i = 0; i < variable_item_size; i++) {
            if (value[i] < '0' || value[i] > '9') {
                log_error("__viewparser_variable_item_complete_index: value[i] is not a digit");
                return 0;
            }

            index = index * 10 + (value[i] - '0');
        }

        viewparser_tag_t* variable = parser->current_tag;
        if (!__viewparser_variable_item_index_create(variable->last_item, index)) {
            log_error("__viewparser_variable_item_complete_index: __viewparser_variable_item_index_create failed");
            return 0;
        }
    }

    bufferdata_reset(&parser->variable_buffer);

    return 1;
}

/**
 * Creates a new variable index for a variable item.
 *
 * @param item A pointer to a variable item.
 * @param value The value of the new index.
 * @return 1 if the index was successfully created, 0 otherwise.
 */
int __viewparser_variable_item_index_create(viewparser_variable_item_t* const item, const int value) {
    if (item == NULL) {
        log_error("__viewparser_variable_item_index_create: item is NULL");
        return 0;
    }

    viewparser_variable_index_t* index = malloc(sizeof * index);
    if (index == NULL) {
        log_error("__viewparser_variable_item_index_create: malloc failed");
        return 0;
    }

    index->next = NULL;
    index->value = value;

    if (item->index == NULL)
        item->index = index;

    if (item->last_index != NULL)
        item->last_index->next = index;

    item->last_index = index;

    return 1;
}

/**
 * Writes a character to the body of a variable in the view parser.
 *
 * @param parser A pointer to a view parser.
 * @param ch The character to write.
 * @return 1 if the character was successfully written, 0 otherwise.
 */
int __viewparser_variable_write_body(viewparser_t* parser, const char ch) {
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
        __viewparser_variable_item_append(variable_item, parser->current_tag);

        break;
    }
    default:
        context->symbol = VIEWPARSER_DEFAULT;
        bufferdata_push(&parser->variable_buffer, ch);
        break;
    }

    return 1;
}

/**
 * Trims trailing whitespace from the parser's variable buffer.
 *
 * @param parser The view parser.
 * @return void
 */
void __viewparser_variable_rtrim(viewparser_t* parser) {
    while (bufferdata_back(&parser->variable_buffer) == ' ')
        bufferdata_pop_back(&parser->variable_buffer);
}

/**
 * Appends a child tag to a parent tag, or adds a sibling to the last child of the parent.
 *
 * @param parent The parent tag to append to or add a sibling to.
 * @param child The child tag to append or add as a sibling.
 * @return void
 */
void __viewparser_tag_append_child_or_sibling(viewparser_tag_t* parent, viewparser_tag_t* child) {
    if (parent == NULL) return;

    if (parent->child == NULL)
        parent->child = child;

    if (parent->last_child != NULL)
        parent->last_child->next = child;

    parent->last_child = child;
}

/**
 * Sets the parent text parameters for a tag.
 *
 * @param tag The tag to set the parent text parameters for.
 * @param parent The parent tag.
 * @return void
 */
void __viewparser_tag_set_parent_text_params(viewparser_tag_t* tag, viewparser_tag_t* parent) {
    if (parent == NULL) return;

    if (parent->last_child != NULL) {
        tag->parent_text_offset = parent->last_child->parent_text_offset + parent->last_child->parent_text_size;
        tag->parent_text_size = bufferdata_writed(&parent->result_content) - tag->parent_text_offset;
    }
    else {
        tag->parent_text_offset = 0;
        tag->parent_text_size = bufferdata_writed(&parent->result_content);
    }
}

/**
 * Returns the data parent tag of a given tag.
 *
 * @param tag The tag to get the data parent of.
 * @return viewparser_tag_t* The data parent tag, or NULL if the tag is NULL.
 */
viewparser_tag_t* __viewparser_get_dataparent(viewparser_tag_t* tag) {
    if (tag == NULL)
        return NULL;

    if (tag->type == VIEWPARSER_TAGTYPE_LOOP)
        return tag;
    
    return tag->data_parent;
}

/**
 * Initializes a viewparser tag.
 *
 * @param tag The tag to initialize.
 * @param type The type of the tag.
 * @param data_parent The data parent of the tag.
 * @param parent The parent of the tag.
 * @return int 1 if successful, 0 otherwise.
 */
void __viewparser_tag_init(viewparser_tag_t* tag, viewparser_tagtype_e type, viewparser_tag_t* data_parent, viewparser_tag_t* parent) {
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
    tag->free = view_tag_free;
}

/**
 * Creates a new root viewparser tag.
 *
 * @return viewparser_tag_t* The newly created root tag, or NULL if memory allocation failed.
 */
viewparser_tag_t* __viewparser_root_tag_create() {
    viewparser_tag_t* tag = malloc(sizeof(viewparser_include_t));
    if (tag == NULL) {
        log_error("__viewparser_root_tag_create: malloc failed\n");
        return NULL;
    }

    __viewparser_tag_init(tag, VIEWPARSER_TAGTYPE_INC, NULL, NULL);
    tag->free = view_tag_include_free;

    return tag;
}

/**
 * Creates a new viewparser include tag.
 *
 * @param parser Pointer to a viewparser object.
 * @return int 1 if the include tag was successfully created, 0 otherwise.
 */
int __viewparser_include_create(viewparser_t* parser) {
    if (parser->level > VIEWPARSER_MAX_DEPTH) {
        log_error("__viewparser_include_create: max depth reached\n");
        return 0;
    }

    viewparser_include_t* tag = malloc(sizeof * tag);
    if (tag == NULL) {
        log_error("__viewparser_include_create: malloc failed\n");
        return 0;
    }

    viewparser_tag_t* current_tag = parser->current_tag;
    viewparser_tag_t* data_parent = __viewparser_get_dataparent(current_tag);

    __viewparser_tag_init((viewparser_tag_t*)tag, VIEWPARSER_TAGTYPE_INC, data_parent, current_tag);
    tag->is_control_key = 0;
    tag->control_key_index = 0;
    tag->base.free = view_tag_include_free;

    __viewparser_tag_set_parent_text_params((viewparser_tag_t*)tag, current_tag);
    __viewparser_tag_append_child_or_sibling(current_tag, (viewparser_tag_t*)tag);

    parser->current_tag = (viewparser_tag_t*)tag;
    parser->level++;

    return 1;
}

/**
 * Write the body of an include tag.
 *
 * @param parser Pointer to a viewparser object.
 * @param ch The character to write.
 * @return int 1 if the character was successfully written, 0 otherwise.
 */
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
            if (__word_include[tag->control_key_index] != ch) {
                log_error("__viewparser_include_write_body: control key mismatch\n");
                return 0;
            }

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

/**
 * Load the content of an include statement.
 *
 * @param parser The view parser.
 * @return Zero if the include statement is invalid or the content cannot be loaded, non-zero otherwise.
 */
int __viewparser_include_load_content(viewparser_t* parser) {
    __viewparser_variable_rtrim(parser);

    bufferdata_complete(&parser->variable_buffer);
    const size_t path_size = bufferdata_writed(&parser->variable_buffer);
    if (path_size >= 4096) {
        log_error("__viewparser_include_load_content: path size exceeds 4096 bytes");
        return 0;
    }

    if (path_size > 0) {
        viewparser_context_t* context = __viewparser_context_create();
        if (context == NULL) {
            log_error("__viewparser_include_load_content: failed to create context for include");
            return 0;
        }

        context->parent = parser->current_context;

        if (parser->current_context->child == NULL)
            parser->current_context->child = context;

        if (parser->current_context->last_child != NULL)
            parser->current_context->last_child->next = context;

        parser->current_context->last_child = context;
        parser->current_context = context;

        const char* path = bufferdata_get(&parser->variable_buffer);
        if (!__viewparser_readfile(parser->current_context, parser->storage_name, path)) {
            log_error("__viewparser_include_load_content: view file not found %s", path);
            return 0;
        }
    }

    bufferdata_reset(&parser->variable_buffer);

    return 1;
}

/**
 * Writes the body of a branch tag.
 *
 * @param parser The view parser.
 * @return int 1 if the body was successfully written, 0 otherwise.
 */
int __viewparser_branch_write_body(viewparser_t* parser) {
    __viewparser_variable_rtrim(parser);

    bufferdata_complete(&parser->variable_buffer);
    const size_t tag_name_size = bufferdata_writed(&parser->variable_buffer);
    const char* tag_name = bufferdata_get(&parser->variable_buffer);

    for (size_t i = 0; i < tag_name_size; i++) {
        const int complete = i + 1 == tag_name_size;

        if (tag_name_size == 2 && __word_if[i] == tag_name[i]) {
            if (complete && !__viewparser_condition_if_create(parser)) {
                log_error("__viewparser_branch_write_body: failed to create if condition");
                return 0;
            }

            parser->current_context->stage = VIEWPARSER_CONDITION_EXPR;
        }
        else if (tag_name_size == 6 && __word_elseif[i] == tag_name[i]) {
            if (complete && !__viewparser_condition_elseif_create(parser)) {
                log_error("__viewparser_branch_write_body: failed to create elseif condition");
                return 0;
            }

            parser->current_context->stage = VIEWPARSER_CONDITION_EXPR;
        }
        else if (tag_name_size == 4 && __word_else[i] == tag_name[i]) {
            if (complete && !__viewparser_condition_else_create(parser)) {
                log_error("__viewparser_branch_write_body: failed to create else condition");
                return 0;
            }

            parser->current_context->stage = VIEWPARSER_CONDITION_EXPR;
        }
        else if (tag_name_size == 5 && __word_endif[i] == tag_name[i]) {
            parser->current_tag = parser->current_tag->parent->parent;
            parser->current_context->stage = VIEWPARSER_BRANCH;
            break;
        }
        else if (tag_name_size == 3 && __word_for[i] == tag_name[i]) {
            if (complete && !__viewparser_loop_create(parser)) {
                log_error("__viewparser_branch_write_body: failed to create loop");
                return 0;
            }

            parser->current_context->stage = VIEWPARSER_LOOP_ITEM;
        }
        else if (tag_name_size == 6 && __word_endfor[i] == tag_name[i]) {
            parser->current_tag = parser->current_tag->parent;
            parser->current_context->stage = VIEWPARSER_BRANCH;
            break;
        }
        else if (complete) {
            log_error("__viewparser_branch_write_body: branch tag not found\n");
            return 0;
        }
    }

    bufferdata_reset(&parser->variable_buffer);

    return 1;
}

/**
 * Creates a new if condition tag and appends it to the current tag.
 *
 * @param parser The view parser.
 * @return int 1 if the tag was successfully created, 0 otherwise.
 */
int __viewparser_condition_if_create(viewparser_t* parser) {
    viewparser_tag_t* tag_cond = malloc(sizeof * tag_cond);
    if (tag_cond == NULL) {
        log_error("malloc failed in __viewparser_condition_if_create\n");
        return 0;
    }

    viewparser_tag_t* current_tag = parser->current_tag;
    viewparser_tag_t* data_parent = __viewparser_get_dataparent(current_tag);

    __viewparser_tag_init(tag_cond, VIEWPARSER_TAGTYPE_COND, data_parent, current_tag);
    tag_cond->free = view_tag_free;
    __viewparser_tag_set_parent_text_params(tag_cond, current_tag);

    viewparser_condition_item_t* tag_if = malloc(sizeof * tag_if);
    if (tag_if == NULL) {
        log_error("malloc failed in __viewparser_condition_if_create\n");
        free(tag_cond);
        return 0;
    }

    __viewparser_tag_init((viewparser_tag_t*)tag_if, VIEWPARSER_TAGTYPE_COND_IF, data_parent, tag_cond);
    tag_if->result = 0;
    tag_if->reverse = 0;
    tag_if->always_true = 0;
    tag_if->base.free = view_tag_condition_free;

    viewparser_variable_item_t* variable_item = __viewparser_variable_item_create();
    if (variable_item == NULL) {
        log_error("__viewparser_variable_item_create failed in __viewparser_condition_if_create\n");
        free(tag_cond);
        free(tag_if);
        return 0;
    }

    __viewparser_condition_item_append(variable_item, tag_if);
    __viewparser_tag_append_child_or_sibling(tag_cond, (viewparser_tag_t*)tag_if);
    __viewparser_tag_append_child_or_sibling(current_tag, tag_cond);

    parser->current_tag = (viewparser_tag_t*)tag_if;

    return 1;
}

/**
 * Appends a new variable item to a condition item.
 *
 * @param variable_item The variable item to append.
 * @param tag The condition item to append the variable item to.
 * @return void
 */
void __viewparser_condition_item_append(viewparser_variable_item_t* variable_item, viewparser_condition_item_t* tag) {
    if (tag->base.item == NULL)
        tag->base.item = variable_item;

    if (tag->base.last_item != NULL)
        tag->base.last_item->next = variable_item;

    tag->base.last_item = variable_item;
}

/**
 * Completes the name of a variable item and resets the variable buffer.
 *
 * @param parser The viewparser instance.
 * @return 1 if the name was successfully completed, 0 otherwise.
 */
int __viewparser_condition_item_complete_name(viewparser_t* parser) {
    __viewparser_variable_rtrim(parser);

    bufferdata_complete(&parser->variable_buffer);
    const size_t variable_item_size = bufferdata_writed(&parser->variable_buffer);
    if (variable_item_size >= VIEWPARSER_VARIABLE_ITEM_NAME_SIZE) {
        log_error("__viewparser_condition_item_complete_name: name size is greater or equal to %zd", VIEWPARSER_VARIABLE_ITEM_NAME_SIZE);
        return 0;
    }

    if (variable_item_size > 0) {
        viewparser_tag_t* condition_item = parser->current_tag;
        strcpy(condition_item->last_item->name, bufferdata_get(&parser->variable_buffer));
        condition_item->last_item->name_length = variable_item_size;
    }

    bufferdata_reset(&parser->variable_buffer);

    return 1;
}

/**
 * Creates a new elseif condition tag and appends it to the current tag.
 *
 * @param parser The view parser instance of type viewparser_t*.
 * @return int 1 if the tag was successfully created, 0 otherwise.
 */
int __viewparser_condition_elseif_create(viewparser_t* parser) {
    viewparser_tag_t* current_tag = parser->current_tag;
    if (current_tag->type != VIEWPARSER_TAGTYPE_COND_IF && current_tag->type != VIEWPARSER_TAGTYPE_COND_ELSEIF) {
        log_error("__viewparser_condition_elseif_create: current tag is not of type COND_IF or COND_ELSEIF");
        return 0;
    }

    viewparser_condition_item_t* tag_elseif = malloc(sizeof * tag_elseif);
    if (tag_elseif == NULL) {
        log_error("__viewparser_condition_elseif_create: malloc failed");
        return 0;
    }

    viewparser_tag_t* tag_cond = parser->current_tag->parent;
    viewparser_tag_t* data_parent = __viewparser_get_dataparent(current_tag);

    __viewparser_tag_init((viewparser_tag_t*)tag_elseif, VIEWPARSER_TAGTYPE_COND_ELSEIF, data_parent, tag_cond);
    tag_elseif->result = 0;
    tag_elseif->reverse = 0;
    tag_elseif->always_true = 0;
    tag_elseif->base.free = view_tag_condition_free;

    viewparser_variable_item_t* variable_item = __viewparser_variable_item_create();
    if (variable_item == NULL) {
        log_error("__viewparser_condition_elseif_create: __viewparser_variable_item_create failed");
        free(tag_elseif);
        return 0;
    }

    __viewparser_condition_item_append(variable_item, tag_elseif);
    __viewparser_tag_append_child_or_sibling(tag_cond, (viewparser_tag_t*)tag_elseif);

    parser->current_tag = (viewparser_tag_t*)tag_elseif;

    return 1;
}

/**
 * Creates a new else condition tag and appends it to the current tag.
 *
 * @param parser The view parser instance of type viewparser_t*.
 * @return int 1 if the tag was successfully created, 0 otherwise.
 */
int __viewparser_condition_else_create(viewparser_t* parser) {
    viewparser_tag_t* current_tag = parser->current_tag;
    if (current_tag->type != VIEWPARSER_TAGTYPE_COND_IF && current_tag->type != VIEWPARSER_TAGTYPE_COND_ELSEIF) {
        log_error("__viewparser_condition_else_create: current tag is not of type COND_IF or COND_ELSEIF");
        return 0;
    }

    viewparser_condition_item_t* tag_else = malloc(sizeof * tag_else);
    if (tag_else == NULL) {
        log_error("__viewparser_condition_else_create: malloc failed");
        return 0;
    }

    viewparser_tag_t* tag_cond = parser->current_tag->parent;
    viewparser_tag_t* data_parent = __viewparser_get_dataparent(current_tag);

    __viewparser_tag_init((viewparser_tag_t*)tag_else, VIEWPARSER_TAGTYPE_COND_ELSE, data_parent, tag_cond);
    tag_else->result = 0;
    tag_else->reverse = 0;
    tag_else->always_true = 1;
    tag_else->base.free = view_tag_condition_free;

    viewparser_variable_item_t* variable_item = __viewparser_variable_item_create();
    if (variable_item == NULL) {
        log_error("__viewparser_condition_else_create: __viewparser_variable_item_create failed");
        free(tag_else);
        return 0;
    }

    __viewparser_condition_item_append(variable_item, tag_else);
    __viewparser_tag_append_child_or_sibling(tag_cond, (viewparser_tag_t*)tag_else);

    parser->current_tag = (viewparser_tag_t*)tag_else;

    return 1;
}

/**
 * Create a loop tag.
 *
 * @param parser The view parser.
 * @return 1 on success, 0 on failure.
 */
int __viewparser_loop_create(viewparser_t* parser) {
    viewparser_loop_t* tag_for = malloc(sizeof * tag_for);
    if (tag_for == NULL) {
        log_error("__viewparser_loop_create: failed to allocate memory for loop tag");
        return 0;
    }

    viewparser_tag_t* current_tag = parser->current_tag;
    viewparser_tag_t* data_parent = __viewparser_get_dataparent(current_tag);

    __viewparser_tag_init((viewparser_tag_t*)tag_for, VIEWPARSER_TAGTYPE_LOOP, data_parent, current_tag);
    __viewparser_tag_set_parent_text_params((viewparser_tag_t*)tag_for, current_tag);
    tag_for->element_name[0] = 0;
    tag_for->key_name[0] = 0;
    tag_for->token = NULL;
    tag_for->base.free = view_tag_loop_free;

    viewparser_variable_item_t* variable_item = __viewparser_variable_item_create();
    if (variable_item == NULL) {
        log_error("__viewparser_loop_create: failed to create variable item");
        free(tag_for);
        return 0;
    }

    __viewparser_variable_item_append(variable_item, (viewparser_tag_t*)tag_for);
    __viewparser_tag_append_child_or_sibling(current_tag, (viewparser_tag_t*)tag_for);

    parser->current_tag = (viewparser_tag_t*)tag_for;

    return 1;
}

/**
 * Write item name to the loop tag.
 *
 * @param parser The view parser.
 * @param ch The character to write.
 * @return 1 on success, 0 on failure.
 */
int __viewparser_loop_write_item(viewparser_t* parser, char ch) {
    viewparser_context_t* const context = parser->current_context;
    viewparser_loop_t* tag_for = (viewparser_loop_t*)parser->current_tag;

    switch (ch)
    {
    case ',':
    case ' ':
    {

        bufferdata_complete(&parser->variable_buffer);
        const size_t tag_name_size = bufferdata_writed(&parser->variable_buffer);
        const char* tag_name = bufferdata_get(&parser->variable_buffer);

        if (tag_name_size > VIEWPARSER_VARIABLE_ITEM_NAME_SIZE - 1) {
            log_error("__viewparser_loop_write_item: tag_name_size > VIEWPARSER_VARIABLE_ITEM_NAME_SIZE - 1\n");
            return 0;
        }

        if (strcmp(tag_name, "in") == 0) {
            log_error("__viewparser_loop_write_item: item not must be named <in> in for\n");
            return 0;
        }

        strcpy(tag_for->element_name, tag_name);

        bufferdata_reset(&parser->variable_buffer);

        if (ch == ',') {
            context->symbol = VIEWPARSER_VARIABLE_SPACE;
            context->stage = VIEWPARSER_LOOP_INDEX;
        }
        else {
            context->symbol = VIEWPARSER_VARIABLE_SPACE;
            context->stage = VIEWPARSER_LOOP_IN;
        }

        break;
    }
    default:
        context->symbol = VIEWPARSER_DEFAULT;
        bufferdata_push(&parser->variable_buffer, ch);
        break;
    }

    return 1;
}

/**
 * Write index name to the loop tag.
 *
 * @param parser The view parser.
 * @param ch The character to write.
 * @return 1 on success, 0 on failure.
 */
int __viewparser_loop_write_index(viewparser_t* parser, char ch) {
    viewparser_context_t* const context = parser->current_context;
    viewparser_loop_t* tag_for = (viewparser_loop_t*)parser->current_tag;

    switch (ch)
    {
    case ' ':
    {
        if (context->symbol == VIEWPARSER_VARIABLE_SPACE)
            break;

        bufferdata_complete(&parser->variable_buffer);
        const size_t tag_name_size = bufferdata_writed(&parser->variable_buffer);
        const char* tag_name = bufferdata_get(&parser->variable_buffer);

        if (tag_name_size > VIEWPARSER_VARIABLE_ITEM_NAME_SIZE - 1) {
            log_error("__viewparser_loop_write_index: tag_name_size > VIEWPARSER_VARIABLE_ITEM_NAME_SIZE - 1\n");
            return 0;
        }

        if (strcmp(tag_name, "in") == 0) {
            log_error("__viewparser_loop_write_index: index not must be named <in> in for\n");
            return 0;
        }

        if (strcmp(tag_name, tag_for->element_name) == 0) {
            log_error("__viewparser_loop_write_index: index name must be different from item name\n");
            return 0;
        }

        strcpy(tag_for->key_name, tag_name);

        bufferdata_reset(&parser->variable_buffer);

        context->symbol = VIEWPARSER_DEFAULT;
        context->stage = VIEWPARSER_LOOP_IN;

        break;
    }
    default:
        context->symbol = VIEWPARSER_DEFAULT;
        bufferdata_push(&parser->variable_buffer, ch);
        break;
    }

    return 1;
}

/**
 * Write 'in' to the loop tag.
 *
 * @param parser The view parser.
 * @param ch The character to write.
 * @return 1 on success, 0 on failure.
 */
int __viewparser_loop_write_in(viewparser_t* parser, char ch) {
    viewparser_context_t* const context = parser->current_context;

    switch (ch)
    {
    case ' ':
    {
        bufferdata_complete(&parser->variable_buffer);
        const size_t tag_name_size = bufferdata_writed(&parser->variable_buffer);
        const char* tag_name = bufferdata_get(&parser->variable_buffer);

        if (tag_name_size > 2) {
            log_error("__viewparser_loop_write_in: tag_name_size > 2\n");
            return 0;
        }

        if (strcmp(tag_name, "in") != 0) {
            log_error("__viewparser_loop_write_in: not found <in> in for\n");
            return 0;
        }

        bufferdata_reset(&parser->variable_buffer);

        context->symbol = VIEWPARSER_DEFAULT;
        context->stage = VIEWPARSER_LOOP_VAR;

        break;
    }
    default:
        context->symbol = VIEWPARSER_DEFAULT;
        bufferdata_push(&parser->variable_buffer, ch);
        break;
    }

    return 1;
}
