#include "json.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static const size_t TOKENS_BUFFER_SIZE = 4096;

void __json_init_parser(jsondoc_t*);
int __json_prepare_parser(jsondoc_t*, const char*);
void __json_insert_symbol(jsonparser_t*);
int __json_alloc_tokens(jsondoc_t*);
jsontok_t* __json_alloc_token(jsondoc_t*);
void __json_init_token(jsondoc_t*, jsontok_t*);
int __json_fill_token(jsontok_t*, const jsontype_t, const int, jsonparser_t*);
void __json_set_child_or_sibling(jsontok_t*, jsontok_t*);
int __json_parse_primitive(jsonparser_t*);
int __json_parse_string(jsonparser_t*);
int __json_stringify_token(jsondoc_t*, jsontok_t*);
void __json_free_stringify(jsonstr_t*);
int __json_stringify_insert(jsondoc_t*, const char*, size_t);
void __json_set_error(jsondoc_t*, const char*);

int json_init(jsondoc_t* document) {
    document->toknext = 0;
    document->tokens_count = 0;
    document->tokens = NULL;

    document->stringify.length = 0;
    document->stringify.pos = 0;
    document->stringify.string = NULL;

    __json_init_parser(document);

    if (__json_alloc_tokens(document) == -1) {
        __json_set_error(document, "Json init error");
        return 0;
    }

    return 1;
}

int json_parse(jsondoc_t* document, const char* string) {
    // if document already init, then return 0
    // if (document->parsed) return 0;

    if (!__json_prepare_parser(document, string)) {
        __json_set_error(document, "json parse error");
        return JSON_ERROR_NOMEM;
    }

    int r;
    int i;
    int count = document->toknext;
    jsontok_t* token;
    jsonparser_t* parser = &document->parser;
    size_t len = strlen(parser->string);

    for (; parser->dirty_pos < len && parser->string[parser->dirty_pos] != '\0'; parser->dirty_pos++) {
        jsontype_t type;
        char c = parser->string[parser->dirty_pos];

        switch (c) {
        case '{':
        case '[':
            count++;
            if (document->tokens == NULL)
                break;

            token = __json_alloc_token(document);
            if (token == NULL)
                return JSON_ERROR_NOMEM;

            token->type = (c == '{' ? JSON_OBJECT : JSON_ARRAY);

            if (parser->toksuper != -1) {
                jsontok_t* t = document->tokens[parser->toksuper];
                /* In strict mode an object or array can't become a key */
                if (t->type == JSON_OBJECT)
                    return JSON_ERROR_INVAL;

                token->parent = parser->toksuper;

                __json_set_child_or_sibling(t, token);
            }
            token->start = parser->dirty_pos;
            parser->toksuper = document->toknext - 1;
            __json_insert_symbol(parser);
            break;
        case '}':
        case ']':
            if (document->tokens == NULL)
                break;

            type = (c == '}' ? JSON_OBJECT : JSON_ARRAY);

            if (document->toknext < 1)
                return JSON_ERROR_INVAL;

            token = document->tokens[document->toknext - 1];
            for (;;) {
                if (token->start != -1 && token->end == -1) {
                    if (token->type != type)
                        return JSON_ERROR_INVAL;

                    token->end = parser->dirty_pos + 1;
                    parser->toksuper = token->parent;
                    break;
                }
                if (token->parent == -1) {
                    if (token->type != type || parser->toksuper == -1)
                        return JSON_ERROR_INVAL;

                    break;
                }
                token = document->tokens[token->parent];
            }
            __json_insert_symbol(parser);

            break;
        case '\"':
            r = __json_parse_string(parser);
            if (r < 0) return r;

            count++;
            if (parser->toksuper != -1 && document->tokens != NULL) {
                jsontok_t* tok = document->tokens[parser->toksuper];
                if (tok->type == JSON_OBJECT || tok->type == JSON_ARRAY)
                    tok->size++;
            }

            break;
        case '\t':
        case '\r':
        case '\n':
        case ' ':
            break;
        case ':':
            parser->toksuper = document->toknext - 1;
            __json_insert_symbol(parser);
            break;
        case ',':
            if (document->tokens != NULL && parser->toksuper != -1 &&
                document->tokens[parser->toksuper]->type != JSON_ARRAY &&
                document->tokens[parser->toksuper]->type != JSON_OBJECT) {
                parser->toksuper = document->tokens[parser->toksuper]->parent;
            }
            __json_insert_symbol(parser);
            break;
        /* In strict mode primitives are: numbers and booleans */
        case '-':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case 't':
        case 'f':
        case 'n':
            /* And they must not be keys of the object */
            if (document->tokens != NULL && parser->toksuper != -1) {
                const jsontok_t* t = document->tokens[parser->toksuper];
                if (t->type == JSON_OBJECT) {
                    return JSON_ERROR_INVAL;
                }
            }

            int r = __json_parse_primitive(parser);
            if (r < 0) return r;

            count++;
            if (parser->toksuper != -1 && document->tokens != NULL) {
                jsontok_t* tok = document->tokens[parser->toksuper];
                if (tok->type == JSON_OBJECT || tok->type == JSON_ARRAY)
                    tok->size++;
            }

            break;

        /* Unexpected char in strict mode */
        default:
            return JSON_ERROR_INVAL;
        }
    }

    if (parser->doc->tokens != NULL) {
        for (i = parser->doc->toknext - 1; i >= 0; i--) {
            /* Unmatched opened object or array */
            if (document->tokens[i]->start != -1 && document->tokens[i]->end == -1)
                return JSON_ERROR_PART;
        }
    }

    return count;
}

void json_free(jsondoc_t* document) {
    jsonparser_t* parser = &document->parser;

    if (document->tokens) {
        for (unsigned int i = 0; i < document->tokens_count; i++) {
            jsontok_t* token = document->tokens[i];
            if (token && token->type == JSON_STRING && token->value.string)
                free(token->value.string);

            free(token);
        }
        free(document->tokens);
    }

    if (parser->string_internal)
        free(parser->string_internal);

    parser->dirty_pos = 0;
    parser->pos = 0;
    parser->toksuper = -1;
    parser->string_len = 0;
    parser->string = NULL;
    parser->string_internal = NULL;

    document->toknext = 0;
    document->tokens_count = 0;
    document->tokens = NULL;
}

jsontok_t* json_doc_token(jsondoc_t* document) {
    if (document->tokens)
        return document->tokens[0];

    return NULL;
}

int json_bool(const jsontok_t* token) {
    return token->value._int;
}

int json_int(const jsontok_t* token) {
    return token->value._int;
}

double json_double(const jsontok_t* token) {
    return token->value._double;
}

long long json_number(const jsontok_t* token) {
    return token->value._long;
}

const char* json_string(const jsontok_t* token) {
    return token->value.string;
}

unsigned int json_uint(const jsontok_t* token) {
    return token->value._uint;
}

int json_is_bool(const jsontok_t* token) {
    return token->type == JSON_BOOL;
}

int json_is_null(const jsontok_t* token) {
    return token->type == JSON_NULL;
}

int json_is_string(const jsontok_t* token) {
    return token->type == JSON_STRING;
}

int json_is_number(const jsontok_t* token) {
    return token->type == JSON_NUMBER;
}

int json_is_int(const jsontok_t* token) {
    return token->type == JSON_INT;
}

int json_is_uint(const jsontok_t* token) {
    return token->type == JSON_UINT;
}

int json_is_double(const jsontok_t* token) {
    return token->type == JSON_DOUBLE;
}

int json_is_object(const jsontok_t* token) {
    return token->type == JSON_OBJECT;
}

int json_is_array(const jsontok_t* token) {
    return token->type == JSON_ARRAY;
}

jsontok_t* json_create_bool(jsondoc_t* document, int value) {
    jsontok_t* token = __json_alloc_token(document);
    if (token == NULL) return NULL;

    token->type = JSON_BOOL;
    token->value._int = value;

    return token;
}

jsontok_t* json_create_null(jsondoc_t* document) {
    jsontok_t* token = __json_alloc_token(document);
    if (token == NULL) return NULL;

    token->type = JSON_NULL;
    token->value._int = 0;

    return token;
}

jsontok_t* json_create_string(jsondoc_t* document, const char* string) {
    jsontok_t* token = __json_alloc_token(document);
    if (token == NULL) return NULL;

    token->type = JSON_STRING;
    token->size = strlen(string);
    token->value.string = malloc(token->size + 1);
    if (token->value.string == NULL) return NULL;

    memcpy(token->value.string, string, token->size);
    token->value.string[token->size] = 0;

    return token;
}

jsontok_t* json_create_number(jsondoc_t* document, long long value) {
    jsontok_t* token = __json_alloc_token(document);
    if (token == NULL) return NULL;

    token->type = JSON_NUMBER;
    token->value._long = value;

    return token;
}

jsontok_t* json_create_int(jsondoc_t* document, int value) {
    jsontok_t* token = __json_alloc_token(document);
    if (token == NULL) return NULL;

    token->type = JSON_INT;
    token->value._int = value;

    return token;
}

jsontok_t* json_create_uint(jsondoc_t* document, unsigned int value) {
    jsontok_t* token = __json_alloc_token(document);
    if (token == NULL) return NULL;

    token->type = JSON_UINT;
    token->value._uint = value;

    return token;
}

jsontok_t* json_create_double(jsondoc_t* document, double value) {
    jsontok_t* token = __json_alloc_token(document);
    if (token == NULL) return NULL;

    token->type = JSON_DOUBLE;
    token->value._double = value;

    return token;
}

jsontok_t* json_create_object(jsondoc_t* document) {
    jsontok_t* token = __json_alloc_token(document);
    if (token == NULL) return NULL;

    token->type = JSON_OBJECT;
    token->value._int = 0;

    return token;
}

jsontok_t* json_create_array(jsondoc_t* document) {
    jsontok_t* token = __json_alloc_token(document);
    if (token == NULL) return NULL;

    token->type = JSON_ARRAY;
    token->value._int = 0;

    return token;
}

int json_array_prepend(jsontok_t* token_array, jsontok_t* token_append) {
    return json_array_append_to(token_array, 0, token_append);
}

int json_array_append(jsontok_t* token_array, jsontok_t* token_append) {
    __json_set_child_or_sibling(token_array, token_append);
    token_array->size++;
    return 1;
}

int json_array_append_to(jsontok_t* token_array, int index, jsontok_t* token_append) {
    jsontok_t* token = token_array->child;
    if (token == NULL) {
        token_array->child = token_append;
        token_array->last_sibling = token_append;
        return 1;
    }

    jsontok_t* token_prev = NULL;
    int find_index = 0;
    int i = 0;
    while (token) {
        if (i == index) {
            find_index = 1;

            if (token_prev == NULL)
                token_array->child = token_append;
            else
                token_prev->sibling = token_append;

            token_append->sibling = token;
            token_array->size++;
            break;
        }

        token_prev = token;
        token = token->sibling;
        i++;
    }

    if (!find_index | (i < index)) {
        __json_set_child_or_sibling(token_array, token_append);
        token_array->size++;
    }

    return 1;
}

int json_array_erase(jsontok_t* token_array, int index, int count) {
    jsontok_t* token = token_array->child;
    if (token == NULL) return 1;
    if (count == 0) return 0;
    if (index < 0 || index >= token_array->size) return 0;

    jsontok_t* token_prev = NULL;
    jsontok_t* token_start = NULL;
    int find_index = 0;
    int i = 0;
    while (token) {
        if (i == index) {
            find_index = 1;
            if (token_prev)
                token_start = token_prev;
        }
        else if (find_index) {
            if (count == i - index) break;
        }

        token_prev = token;
        token = token->sibling;
        i++;
    }

    if (!find_index) return 0;

    if (token_start == NULL)
        token_array->child = token;
    else
        token_start->sibling = token;

    token_array->size = token_array->size - (i - index);

    return 1;
}

int json_array_clear(jsontok_t* token_array) {
    token_array->child = NULL;
    token_array->last_sibling = NULL;
    token_array->size = 0;
    return 1;
}

int json_array_size(jsontok_t* token_array) {
    return token_array->size;
}

jsontok_t* json_array_get(jsontok_t* token_array, int index) {
    if (token_array == NULL) return NULL;

    jsontok_t* token = token_array->child;
    if (token == NULL) return NULL;

    int i = 0;
    while (token) {
        if (i == index) return token;

        token = token->sibling;
        i++;
    }

    return NULL;
}

int json_object_set(jsontok_t* token_object, const char* key, jsontok_t* token) {
    jsontok_t* token_key = json_create_string(token_object->doc, key);
    if (token_key == NULL) return 0;

    __json_set_child_or_sibling(token_key, token);
    __json_set_child_or_sibling(token_object, token_key);

    token_object->size++;

    return 1;
}

jsontok_t* json_object_get(jsontok_t* token_object, const char* key) {
    if (token_object == NULL) return NULL;

    jsontok_t* token = token_object->child;
    if (token == NULL) return NULL;

    int i = 0;
    while (token) {
        if (strcmp(json_string(token), key) == 0) return token->child;

        token = token->sibling;
        i++;
    }

    return NULL;
}

int json_object_remove(jsontok_t* token_object, const char* key) {
    if (token_object == NULL) return 0;

    jsontok_t* token_prev = NULL;
    jsontok_t* token = token_object->child;
    if (token == NULL) return 0;

    int i = 0;
    while (token) {
        if (strcmp(json_string(token), key) == 0) {
            if (token_prev)
                token_prev->sibling = token->sibling;
            else
                token_object->child = token->sibling;

            token_object->size--;
            return 1;
        }

        token_prev = token;
        token = token->sibling;
        i++;
    }

    return 0;
}

int json_object_size(jsontok_t* token_object) {
    return token_object->size;
}

int json_object_clear(jsontok_t* token_object) {
    token_object->child = NULL;
    token_object->last_sibling = NULL;
    token_object->size = 0;
    return 1;
}

void json_token_set_bool(jsontok_t* token, int value) {
    token->child = NULL;
    token->type = JSON_BOOL;
    token->size = 0;
    token->value._int = value ? 1 : 0;
}

void json_token_set_null(jsontok_t* token) {
    token->child = NULL;
    token->type = JSON_NULL;
    token->size = 0;
    token->value._int = 0;
}

void json_token_set_string(jsontok_t* token, const char* value) {
    if (token->value.string)
        free(token->value.string);

    const char* string = value;

    token->child = NULL;
    token->type = JSON_STRING;
    token->size = strlen(string);
    token->value.string = malloc(token->size + 1);
    if (token->value.string == NULL)
        return;

    memcpy(token->value.string, string, token->size);
    token->value.string[token->size] = 0;
}

void json_token_set_number(jsontok_t* token, long long value) {
    token->child = NULL;
    token->type = JSON_NUMBER;
    token->size = 0;
    token->value._long = value;
}

void json_token_set_int(jsontok_t* token, int value) {
    token->child = NULL;
    token->type = JSON_INT;
    token->size = 0;
    token->value._int = value;
}

void json_token_set_uint(jsontok_t* token, unsigned int value) {
    token->child = NULL;
    token->type = JSON_UINT;
    token->size = 0;
    token->value._uint = value;
}

void json_token_set_double(jsontok_t* token, double value) {
    token->child = NULL;
    token->type = JSON_DOUBLE;
    token->size = 0;
    token->value._double = value;
}

void json_token_set_object(jsontok_t* token, jsontok_t* token_object) {
    token->child = token_object->child;
    token->last_sibling = token_object->last_sibling;
    token->type = JSON_OBJECT;
    token->size = token_object->size;
    token->value._int = 0;
    token->start = -1;
    token->end = -1;
}

void json_token_set_array(jsontok_t* token, jsontok_t* token_array) {
    token->child = token_array->child;
    token->last_sibling = token_array->last_sibling;
    token->type = JSON_ARRAY;
    token->size = token_array->size;
    token->value._int = 0;
    token->start = -1;
    token->end = -1;
}

jsonit_t json_init_it(jsontok_t* token) {
    jsonit_t it = {
        .ok = 0,
        .size = 0,
        .index = 0,
        .type = token->type,
        .key._int = 0,
        .value = NULL,
        .parent = NULL
    };

    if (token->type == JSON_OBJECT) {
        it.ok = 1;
        it.key.string = token->child->value.string;
        it.value = token->child->child;
    }
    else if (token->type == JSON_ARRAY) {
        it.ok = 1;
        it.key._int = 0;
        it.value = token->child;
    }
    it.parent = token->child;
    it.size = token->size;

    return it;
}

int json_end_it(jsonit_t* iterator) {
    return iterator->index == iterator->size;
}

const void* json_it_key(jsonit_t* iterator) {
    if (iterator->type == JSON_OBJECT)
        return iterator->key.string;
    else if (iterator->type == JSON_ARRAY)
        return &iterator->key._int;

    return NULL;
}

jsontok_t* json_it_value(jsonit_t* iterator) {
    return iterator->value;
}

jsonit_t json_next_it(jsonit_t* iterator) {
    iterator->index++;

    if (json_end_it(iterator)) return *iterator;

    iterator->parent = iterator->parent->sibling;

    if (iterator->type == JSON_OBJECT) {
        iterator->key.string = iterator->parent->value.string;
        iterator->value = iterator->parent->child;
    }
    else if (iterator->type == JSON_ARRAY) {
        iterator->key._int = 0;
        iterator->value = iterator->parent;
    }

    return *iterator;
}

char* json_stringify(jsondoc_t* document) {
    jsontok_t* token = document->tokens[0];

    __json_free_stringify(&document->stringify);

    if (token->type != JSON_OBJECT && token->type != JSON_ARRAY)
        return NULL;

    if (!__json_stringify_token(document, token)) {
        __json_free_stringify(&document->stringify);
    }

    document->stringify.string[document->stringify.pos] = 0;

    return document->stringify.string;
}

void __json_set_error(jsondoc_t* document, const char* string) {
    document->ok = 0;
    strncpy(document->error, string, JSON_ERROR_BUFFER_SIZE);
}

void __json_init_parser(jsondoc_t* document) {
    jsonparser_t* parser = &document->parser;

    parser->dirty_pos = 0;
    parser->pos = 0;
    parser->toksuper = -1;
    parser->string_internal = NULL;
    parser->string = NULL;
    parser->string_len = 0;
    parser->doc = document;
}

int __json_prepare_parser(jsondoc_t* document, const char* string) {
    size_t string_len = strlen(string);
    jsonparser_t* parser = &document->parser;

    parser->string_internal = malloc(string_len + 1);
    if (parser->string_internal == NULL) return 0;

    parser->string = string;
    parser->string_len = string_len;

    return 1;
}

void __json_insert_symbol(jsonparser_t* parser) {
    parser->string_internal[parser->pos] = parser->string[parser->dirty_pos];
    parser->pos++;
}

/**
 * Increase\decrease token pool
 * TODO: create array of tokens after array of pointers.
 * struct token_array {
 *   jsontok_t* array;
 *   struct token_array* next;
 * }
 */
int __json_alloc_tokens(jsondoc_t *document) {
    size_t tokens_count = document->tokens_count + TOKENS_BUFFER_SIZE;
    jsontok_t** tokens = realloc(document->tokens, sizeof(jsontok_t*) * tokens_count);
    if (tokens == NULL) return 0;

    for (size_t i = document->tokens_count; i < tokens_count; i++) {
        tokens[i] = NULL;
    }

    document->tokens = tokens;
    document->tokens_count = tokens_count;

    return 1;
}

/**
 * Allocates a fresh unused token from the token pool.
 */
jsontok_t* __json_alloc_token(jsondoc_t* document) {
    if (document->toknext >= document->tokens_count)
        if (__json_alloc_tokens(document) == -1)
            return NULL;

    jsontok_t* token = malloc(sizeof * token);
    __json_init_token(document, token);

    document->tokens[document->toknext] = token;
    document->toknext++;

    return token;
}

void __json_init_token(jsondoc_t* document, jsontok_t* token) {
    token->start = -1;
    token->end = -1;
    token->size = 0;
    token->parent = -1;
    token->child = NULL;
    token->sibling = NULL;
    token->last_sibling = NULL;
    token->value._int = 0;
    token->doc = document;
}

/**
 * Fills token type and boundaries.
 * TODO: avoid string alloc, set token string from parser->string_internal.
 * Add for the token field-flag string_allocated
 */
int __json_fill_token(jsontok_t* token, const jsontype_t type, const int start, jsonparser_t* parser) {
    token->type = type;
    token->start = start;
    token->end = parser->pos;
    parser->string_internal[parser->pos] = 0;
    const char* string = &parser->string_internal[start];

    switch (type) {
    case JSON_UNDEFINED:
        break;
    case JSON_OBJECT:
        {
            token->size = 0;
            token->value._int = 0;
        }
        break;
    case JSON_ARRAY:
        {
            token->size = 0;
            token->value._int = 0;
        }
        break;
    case JSON_STRING:
        {
            token->size = parser->pos - start;
            token->value.string = malloc(token->size + 1);
            if (token->value.string == NULL)
                return JSON_ERROR_NOMEM;

            memcpy(token->value.string, string, token->size);
            token->value.string[token->size] = 0;
        }
        break;
    case JSON_BOOL:
        {
            token->size = 0;
            token->value._int = string[0] == 't' ? 1 : 0;
        }
        break;
    case JSON_NULL:
        {
            token->size = 0;
            token->value._int = 0;
        }
        break;
    case JSON_INT:
        {
            token->size = 0;
            long long lng = atoll(string);

            if (lng >= -2147483648 && lng <= 2147483647) {
                token->type = JSON_INT;
                token->value._int = (int)lng;
            }
            else if (lng >= 0 && lng <= 4294967295) {
                token->type = JSON_UINT;
                token->value._uint = (unsigned int)lng;
            }
            else {
                token->type = JSON_NUMBER;
                token->value._long = lng;
            }
        }
        break;
    case JSON_NUMBER:
    case JSON_UINT:
        break;
    case JSON_DOUBLE:
        {
            token->size = 0;

            char* end = NULL;
            token->value._double = strtod(string, &end);
        }
        break;
    }

    return 1;
}

void __json_set_child_or_sibling(jsontok_t* token_src, jsontok_t* token_dst) {
    if (!token_src->child)
        token_src->child = token_dst;
    else
        token_src->last_sibling->sibling = token_dst;

    token_src->last_sibling = token_dst;
}

/**
 * Fills next available token with JSON primitive.
 */
int __json_parse_primitive(jsonparser_t* parser) {
    jsontok_t *token;
    unsigned int start = parser->dirty_pos;
    int start_internal = parser->pos;
    jsontype_t type = JSON_UNDEFINED;
    int is_signed = 0;
    int is_double = 0;
    int is_int = 0;
    int is_bool = 0;
    int is_null = 0;

    for (; parser->dirty_pos < parser->string_len && parser->string[parser->dirty_pos] != '\0'; parser->dirty_pos++) {
        char ch = parser->string[parser->dirty_pos];
        switch (ch) {
        case '-':
          if (parser->dirty_pos != start) {
            parser->dirty_pos = start;
            return JSON_ERROR_INVAL;
          }
          is_signed = 1;
          break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
          is_int = 1;
          break;
        case '.':
          is_double = 1;
          break;
        case 't':
        case 'f':
          is_bool = 1;
          break;
        case 'n':
          is_null = 1;
          break;
        default:
          // to quiet a warning from gcc
          break;
        }

        switch (ch) {
        /* In strict mode primitive must be followed by "," or "}" or "]" */
        case ':':
        case '\t':
        case '\r':
        case '\n':
        case ' ':
        case ',':
        case ']':
        case '}':
            goto found;
        default:
            // to quiet a warning from gcc
            break;
        }
        if (ch < 32 || ch >= 127) {
            parser->dirty_pos = start;
            return JSON_ERROR_INVAL;
        }

        __json_insert_symbol(parser);
    }

    /* In strict mode primitive must be followed by a comma/object/array */
    parser->dirty_pos = start;
    return JSON_ERROR_PART;

    found:
    if (is_double && !is_bool && !is_null) {
        type = JSON_DOUBLE;
    }
    else if (is_bool && !is_int && !is_null) {
        type = JSON_BOOL;
    }
    else if (is_null && !is_int && !is_bool) {
        type = JSON_NULL;
    }
    else if (is_int && !is_bool && !is_null) {
        type = JSON_INT;
        int length = parser->pos - start_internal;
        if (length > 19 + is_signed) {
            return JSON_ERROR_INVAL;
        }
    }

    if (parser->doc->tokens == NULL) {
        parser->dirty_pos--;
        return 0;
    }
    token = __json_alloc_token(parser->doc);
    if (token == NULL) {
        parser->dirty_pos = start;
        return JSON_ERROR_NOMEM;
    }

    __json_fill_token(token, type, start_internal, parser);
    token->parent = parser->toksuper;
    parser->dirty_pos--;

    jsontok_t* t = parser->doc->tokens[parser->toksuper];
    __json_set_child_or_sibling(t, token);

    /* Skip symbol, because json_fill_token write \0 to end string */
    // parser->pos++;

    return 0;
}

/**
 * Fills next token with JSON string.
 */
int __json_parse_string(jsonparser_t* parser) {
    int start = parser->dirty_pos;
    int start_internal = parser->pos;

    __json_insert_symbol(parser);

    /* Skip starting quote */
    parser->dirty_pos++;
  
    for (; parser->dirty_pos < parser->string_len && parser->string[parser->dirty_pos] != '\0'; parser->dirty_pos++) {
        char c = parser->string[parser->dirty_pos];

        /* Quote: end of string */
        if (c == '\"') {
            if (parser->doc->tokens == NULL)
                return 0;

            jsontok_t* token = __json_alloc_token(parser->doc);

            if (token == NULL) {
                parser->dirty_pos = start;
                return JSON_ERROR_NOMEM;
            }

            __json_fill_token(token, JSON_STRING, start_internal + 1, parser);

            token->parent = parser->toksuper;

            jsontok_t* t = parser->doc->tokens[parser->toksuper];
            __json_set_child_or_sibling(t, token);

            /* Skip symbol, because json_fill_token write \0 to end string */
            // parser->pos++;

            return 0;
        }

        /* Backslash: Quoted symbol expected */
        if (c == '\\' && parser->dirty_pos + 1 < parser->string_len) {
            parser->dirty_pos++;
            switch (parser->string[parser->dirty_pos]) {
            /* Allowed escaped symbols */
            case '\"':
            case '/':
            case 'b':
            case 'f':
            case 'r':
            case 'n':
            case 't':
                break;
            case '\\':
                break;
            /* Allows escaped symbol \uXXXX */
            case 'u':
                parser->dirty_pos++;
                for (int i = 0; i < 4 && parser->dirty_pos < parser->string_len && parser->string[parser->dirty_pos] != '\0'; i++) {
                    /* If it isn't a hex character we have an error */
                    if (!((parser->string[parser->dirty_pos] >= 48 && parser->string[parser->dirty_pos] <= 57) ||   /* 0-9 */
                            (parser->string[parser->dirty_pos] >= 65 && parser->string[parser->dirty_pos] <= 70) ||   /* A-F */
                            (parser->string[parser->dirty_pos] >= 97 && parser->string[parser->dirty_pos] <= 102))) { /* a-f */
                        parser->dirty_pos = start;
                        return JSON_ERROR_INVAL;
                    }
                    parser->dirty_pos++;
                }
                parser->dirty_pos--;
                break;
            /* Unexpected symbol */
            default:
                parser->dirty_pos = start;
                return JSON_ERROR_INVAL;
            }
        }

        __json_insert_symbol(parser);
    }
    parser->dirty_pos = start;

    return JSON_ERROR_PART;
}

int __json_stringify_token(jsondoc_t* document, jsontok_t* token) {
    switch (token->type) {
    case JSON_UNDEFINED:
        return 0;
    case JSON_OBJECT:
        {
            jsontok_t* t = token->child;
            if (!__json_stringify_insert(document, "{", 1)) return 0;

            while (t) {
                // write object key
                if (!__json_stringify_insert(document, "\"", 1)) return 0;
                if (!__json_stringify_insert(document, t->value.string, t->size)) return 0;
                if (!__json_stringify_insert(document, "\":", 2)) return 0;

                if (!__json_stringify_token(document, t->child)) return 0;

                if (t->sibling) {
                    if (!__json_stringify_insert(document, ",", 1)) return 0;
                }

                t = t->sibling;
            }

            if (!__json_stringify_insert(document, "}", 1)) return 0;
        }
        break;
    case JSON_ARRAY:
        {
            jsontok_t* t = token->child;
            if (!__json_stringify_insert(document, "[", 1)) return 0;

            while (t) {
                if (!__json_stringify_token(document, t)) return 0;

                if (t->sibling) {
                    if (!__json_stringify_insert(document, ",", 1)) return 0;
                }

                t = t->sibling;
            }

            if (!__json_stringify_insert(document, "]", 1)) return 0;
        }
        break;
    case JSON_STRING:
        {
            const char* value = token->value.string;
            int size = token->size;

            if (!__json_stringify_insert(document, "\"", 1)) return 0;
            if (!__json_stringify_insert(document, value, size)) return 0;
            if (!__json_stringify_insert(document, "\"", 1)) return 0;
        }
        break;
    case JSON_BOOL:
        {
            const char* value = token->value._int ? "true" : "false";
            int size = token->value._int ? 4 : 5;

            if (!__json_stringify_insert(document, value, size)) return 0;
        }
        break;
    case JSON_NULL:
        {
            const char* value = "null";
            int size = 4;

            if (!__json_stringify_insert(document, value, size)) return 0;
        }
        break;
    case JSON_NUMBER:
        {
            size_t buffer_size = 32;
            char buffer[buffer_size];
            size_t size = snprintf(buffer, buffer_size - 1, "%lld", token->value._long);

            if (!__json_stringify_insert(document, buffer, size)) return 0;
        }
        break;
    case JSON_INT:
        {
            size_t buffer_size = 16;
            char buffer[buffer_size];
            size_t size = snprintf(buffer, buffer_size - 1, "%d", token->value._int);

            if (!__json_stringify_insert(document, buffer, size)) return 0;
        }
        break;
    case JSON_UINT:
        {
            size_t buffer_size = 16;
            char buffer[buffer_size];
            size_t size = snprintf(buffer, buffer_size - 1, "%u", token->value._uint);

            if (!__json_stringify_insert(document, buffer, size)) return 0;
        }
        break;
    case JSON_DOUBLE:
        {
            size_t buffer_size = 32;
            char buffer[buffer_size];
            size_t size = snprintf(buffer, buffer_size - 1, "%f", token->value._double);

            if (!__json_stringify_insert(document, buffer, size)) return 0;
        }
        break;
    }

    return 1;
}

void __json_free_stringify(jsonstr_t* stringify) {
    stringify->length = 0;
    stringify->pos = 0;

    if (stringify->string)
        free(stringify->string);

    stringify->string = NULL;
}

// TODO: optimize realloc on 4K buffer
int __json_stringify_insert(jsondoc_t* document, const char* string, size_t length) {
    if (document->stringify.string == NULL ||
        document->stringify.length < length + document->stringify.pos
    ) {
        document->stringify.length += length;

        char* data = realloc(document->stringify.string, document->stringify.length + 1);
        if (data == NULL) return 0;

        document->stringify.string = data;
    }

    memcpy(&document->stringify.string[document->stringify.pos], string, length);

    document->stringify.pos += length;

    return 1;
}
