#include <string.h>
#include "formdataparser.h"

int formdataparser_add_entity(formdataparser_t*, int);

void formdataparser_init(formdataparser_t* parser, size_t payload_size) {
    parser->type = NULL;
    parser->quote = 0;
    parser->cmp = 0;
    parser->prev_symbol = 0;
    parser->type_size = 0;
    parser->payload_size = payload_size;
    parser->payload_offset = 0;
    parser->offset = 0;
    parser->size = 0;
    parser->field = NULL;
    parser->last_field = NULL;
}

void formdataparser_free(formdataparser_t* parser) {
    if (parser->type) free(parser->type);

    formdatafield_t* field = parser->field;
    while (field) {
        formdatafield_t* next = field->next;

        if (field->key) free(field->key);

        free(field);

        field = next;
    }
}

void formdataparser_parse(formdataparser_t* parser, const char* buffer, size_t buffer_size) {
    for (size_t i = 0; i < buffer_size; i++) {
        switch (buffer[i]) {
        case ' ':
            if (!parser->quote) {
                parser->payload_offset++;
                continue;
            }
            parser->size++;
            break;
        case ';':
            if (!parser->quote) {
                if (formdataparser_add_entity(parser, FORMDATAVALUE) == -1) return;
                parser->cmp = 0;
                parser->payload_offset++;
                continue;
            }
            parser->size++;
            break;
        case '=':
            if (parser->quote) break;
            else if (parser->cmp) {
                parser->size++;
                break;
            }

            if (formdataparser_add_entity(parser, FORMDATAKEY) == -1) return;
            parser->cmp = 1;
            parser->payload_offset++;
            continue;
        case '"':
            if (!parser->quote) {
                parser->quote = 1;
                parser->payload_offset++;
                parser->offset++;
                continue;
            }
            else if (parser->prev_symbol != '\\') {
                parser->quote = 0;
            }
            else {
                parser->size++;
            }
            break;
        default:
            parser->size++;
            break;
        }

        parser->prev_symbol = buffer[i];
        if (parser->size < FORMDATABUFSIZ) {
            parser->buffer[parser->size - 1] = buffer[i];
        }
        parser->payload_offset++;
    }

    if (parser->payload_offset == parser->payload_size) {
        formdataparser_add_entity(parser, FORMDATAVALUE);
    }
}

formdatalocation_t formdataparser_field(formdataparser_t* parser, const char* field) {
    formdatalocation_t result = {
        .ok = 0,
        .offset = 0,
        .size = 0
    };
    formdatafield_t* pfield = parser->field;

    while (pfield) {
        if (strcmp(pfield->key, field) == 0) {
            result.ok = 1;
            result.offset = pfield->value_offset;
            result.size = pfield->value_size;
            break;
        }

        pfield = pfield->next;
    }

    return result;
}

formdatafield_t* formdataparser_fields(formdataparser_t* parser) {
    return parser->field;
}

int formdataparser_add_entity(formdataparser_t* parser, int type) {
    if (parser->type == NULL) {
        if (parser->size >= FORMDATABUFSIZ) return -1;

        parser->type = malloc(parser->size + 1);
        if (!parser->type) return -1;

        strncpy(parser->type, parser->buffer, parser->size);
    }
    else if (type == FORMDATAKEY) {
        if (parser->size >= FORMDATABUFSIZ) return -1;

        formdatafield_t* field = malloc(sizeof(formdatafield_t));
        if (!field) return -1;

        if (!parser->field) {
            parser->field = field;
        }
        if (parser->last_field) {
            parser->last_field->next = field;
        }
        parser->last_field = field;

        field->value_offset = 0;
        field->value_size = 0;
        field->next = NULL;
        field->key_size = parser->size;
        field->key = malloc(parser->size + 1);
        if (!field->key) return -1;

        strncpy(field->key, parser->buffer, parser->size);
        field->key[parser->size] = 0;
    }
    else if (type == FORMDATAVALUE) {
        if (parser->last_field->key == NULL) return -1;

        parser->last_field->value_offset = parser->offset;
        parser->last_field->value_size = parser->size;
    }

    parser->offset = parser->payload_offset + 1;
    parser->size = 0;

    return 0;
}
