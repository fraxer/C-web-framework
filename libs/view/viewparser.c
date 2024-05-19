#include "storage.h"
#include "log.h"
#include "viewparser.h"

static int __viewparser_readfile(viewparser_t* parser, const char* path);
static int __viewparser_parse_content(viewparser_t* parser);
static int __viewparser_variable_create(viewparser_t* parser);
static void __viewparser_variable_free(viewparser_variable_t* variable);
static void __viewparser_variable_last_remove(viewparser_t* parser);
static int __viewparser_variable_item_create(viewparser_variable_t* variable);
static void __viewparser_variable_item_free(viewparser_variable_item_t* item);
static int __viewparser_variable_item_complete(viewparser_t* parser);
static int __viewparser_replace_tag_on_value(viewparser_t* parser);

viewparser_t* viewparser_init(const jsondoc_t* document, const char* storage_name, const char* path) {
    viewparser_t* parser = malloc(sizeof * parser);
    if (parser == NULL) return NULL;

    parser->document = document;
    parser->storage_name = storage_name;
    parser->path = path;

    parser->buffer = NULL;
    parser->stage = VIEWPARSER_TEXT;
    parser->symbol = VIEWPARSER_DEFAULT;
    parser->bytes_readed = 0;
    parser->pos_start = 0;
    parser->pos = 0;
    parser->variable = NULL;
    parser->last_variable = NULL;

    bufferdata_init(&parser->variable_buffer);
    bufferdata_init(&parser->buf);

    return parser;
}

int viewparser_run(viewparser_t* parser) {
    if (!__viewparser_readfile(parser, parser->path)) {
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

    free(parser);
}

int __viewparser_readfile(viewparser_t* parser, const char* path) {
    file_t file = storage_file_get(parser->storage_name, path);
    if (!file.ok)
        return 0;

    parser->buffer = file.content(&file);
    parser->bytes_readed = file.size;

    file.close(&file);

    return 1;
}

int __viewparser_parse_content(viewparser_t* parser) {
    for (parser->pos = parser->pos_start; parser->pos < parser->bytes_readed; parser->pos++) {
        const char ch = parser->buffer[parser->pos];

        switch (parser->stage)
        {
        case VIEWPARSER_TEXT:
        {
            switch (ch)
            {
            case '{':
            {
                if (parser->symbol == VIEWPARSER_FIGFIGOPEN) {
                    parser->stage = VIEWPARSER_VARIABLE;
                    if (!__viewparser_variable_create(parser))
                        return 0;

                    if (!__viewparser_variable_item_create(parser->last_variable))
                        return 0;

                    bufferdata_pop_back(&parser->buf);
                    break;
                }

                parser->symbol = VIEWPARSER_FIGFIGOPEN;
                bufferdata_push(&parser->buf, ch);

                break;
            }
            default:
                bufferdata_push(&parser->buf, ch);
                break;
            }

            break;
        }
        case VIEWPARSER_VARIABLE:
        {
            switch (ch)
            {
            case '}':
            {
                if (parser->symbol == VIEWPARSER_FIGFIGCLOSE) {
                    parser->stage = VIEWPARSER_TEXT;
                    break;
                }

                if (parser->symbol == VIEWPARSER_VARIABLE_DOT)
                    return 0;

                parser->symbol = VIEWPARSER_FIGFIGCLOSE;

                if (!__viewparser_variable_item_complete(parser))
                    return 0;

                if (!__viewparser_replace_tag_on_value(parser))
                    return 0;

                break;
            }
            case ' ':
            {
                if (parser->symbol == VIEWPARSER_FIGFIGOPEN)
                    break;

                bufferdata_push(&parser->variable_buffer, ch);

                break;
            }
            case '[':
            {
                if (parser->symbol == VIEWPARSER_FIGFIGOPEN)
                    return 0;

                break;
            }
            case ']':
            {
                if (parser->symbol == VIEWPARSER_FIGFIGOPEN)
                    return 0;

                break;
            }
            case '.':
            {
                if (parser->symbol == VIEWPARSER_FIGFIGOPEN)
                    return 0;
                if (parser->symbol == VIEWPARSER_FIGPERCENTOPEN)
                    return 0;
                if (parser->symbol == VIEWPARSER_FIGASTOPEN)
                    return 0;
                if (parser->symbol == VIEWPARSER_VARIABLE_DOT)
                    return 0;

                parser->symbol = VIEWPARSER_VARIABLE_DOT;

                if (!__viewparser_variable_item_complete(parser))
                    return 0;

                if (!__viewparser_variable_item_create(parser->last_variable))
                    return 0;

                break;
            }
            default:
                parser->symbol = VIEWPARSER_DEFAULT;
                bufferdata_push(&parser->variable_buffer, ch);
                break;
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

    variable->json_token = json_root(parser->document);
    variable->item = NULL;
    variable->last_item = NULL;
    variable->next = NULL;

    if (parser->variable == NULL)
        parser->variable = variable;

    if (parser->last_variable != NULL)
        parser->last_variable->next = variable;

    parser->last_variable = variable;

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
    variable->next = NULL;

    free(variable);
}

void __viewparser_variable_last_remove(viewparser_t* parser) {
    viewparser_variable_t* variable = parser->variable;
    viewparser_variable_t* last_variable = parser->last_variable;
    if (variable == last_variable) {
        parser->variable = NULL;
        parser->last_variable = NULL;
    }
    else {
        while (variable) {
            if (variable->next == parser->last_variable) {
                variable->next = NULL;
                parser->last_variable = variable;
                break;
            }

            variable = variable->next;
        }
    }

    __viewparser_variable_free(last_variable);
}

int __viewparser_variable_item_create(viewparser_variable_t* variable) {
    if (variable == NULL) return 0;

    viewparser_variable_item_t* item = malloc(sizeof * item);
    if (item == NULL) return 0;

    item->index = 0;
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

    free(item);
}

int __viewparser_variable_item_complete(viewparser_t* parser) {
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
        const int index = 0;
        parser->last_variable->last_item->index = index;
        strcpy(parser->last_variable->last_item->name, bufferdata_get(&parser->variable_buffer));
        parser->last_variable->last_item->name_length = variable_item_size;
    }

    bufferdata_reset(&parser->variable_buffer);

    return 1;
}

int __viewparser_replace_tag_on_value(viewparser_t* parser) {
    // const jsontok_t* object = json_root(parser->document);
    // if (!json_is_object(object))
    //     return 0;

    viewparser_variable_item_t* item = parser->last_variable->item;
    const jsontok_t* json_token = parser->last_variable->json_token;
    while (item) {
        const jsontok_t* token = json_object_get(json_token, item->name);
        if (token == NULL) break;

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

    __viewparser_variable_last_remove(parser);

    return 1;
}
