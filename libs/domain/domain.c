#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "domain.h"

typedef struct domain_parser {
    char* template;
    char* prepared_template;
    size_t pos;
    size_t pcre_pos;
    size_t prepared_pos;
    int brackets_count;
} domain_parser_t;

int domain_estimate_length(const char*);

void domain_parser_alloc(domain_parser_t*, domain_t*);

void domain_parser_insert_symbol(domain_parser_t* parser);

void domain_parser_insert_custom_symbol(domain_parser_t* parser, char ch);

domain_t* domain_create(const char* value) {
    domain_t* result = NULL;
    domain_t* domain = domain_alloc(value);

    if (domain == NULL) goto failed;

    if (domain_parse(domain) == -1) goto failed;

    domain->pcre_template = pcre_compile(domain->prepared_template, 0, &domain->pcre_error, &domain->pcre_erroffset, NULL);

    if (domain->pcre_error != NULL) goto failed;

    result = domain;

    failed:

    if (result == NULL) {
        if (domain != NULL) domain_free(domain);
    }

    return result;
}

void domain_free(domain_t* domain) {
    domain_t* current = domain;

    while (current) {
        domain_t* next = current->next;

        if (current->template) free(current->template);
        if (current->prepared_template) free(current->prepared_template);

        pcre_free(current->pcre_template);

        free(current);

        current = next;
    }
}

domain_t* domain_alloc(const char* value) {
    domain_t* result = NULL;

    domain_t* domain = (domain_t*)malloc(sizeof(domain_t));

    if (domain == NULL) goto failed;


    domain->template = (char*)malloc(strlen(value) + 1);

    if (domain->template == NULL) goto failed;

    strcpy(domain->template, value);


    int pcre_length = domain_estimate_length(domain->template);
    if (pcre_length == -1) goto failed;

    domain->prepared_template = (char*)malloc(pcre_length + 1);

    if (domain->prepared_template == NULL) goto failed;

    domain->pcre_template = NULL;
    domain->next = NULL;

    result = domain;

    failed:

    if (result == NULL) {
        if (domain->template != NULL) free(domain->template);
        if (domain != NULL) free(domain);
    }

    return result;
}

int domain_parse(domain_t* domain) {
    domain_parser_t parser;

    domain_parser_alloc(&parser, domain);

    size_t length = strlen(domain->template);

    if (domain->template[0] != '^' && domain->template[length - 1] != '$') {
        parser.prepared_pos = 1;
    }

    for (; parser.pos < length; parser.pos++) {
        switch (domain->template[parser.pos]) {
        case '(':
        case '[':
            parser.brackets_count++;
            domain_parser_insert_symbol(&parser);
            break;
        case ')':
        case ']':
            if (parser.brackets_count == 0) return -1;
            parser.brackets_count--;
            domain_parser_insert_symbol(&parser);
            break;
        case '*':
            if (parser.brackets_count > 0) {
                domain_parser_insert_symbol(&parser);
                break;
            }

            if (parser.pos == 0) {
                domain_parser_insert_custom_symbol(&parser, '.');
                domain_parser_insert_symbol(&parser);
            }
            else if (parser.pos == parser.pcre_pos - 1) {
                domain_parser_insert_custom_symbol(&parser, '.');
                domain_parser_insert_symbol(&parser);
            }
            else {
                log_error("Domain error: Asterisk must be in start or end of the string\n");
                return -1;
            }
            break;
        case '.':
            if (parser.brackets_count > 0) {
                domain_parser_insert_symbol(&parser);
                break;
            }

            domain_parser_insert_custom_symbol(&parser, '\\');
            domain_parser_insert_symbol(&parser);
            break;
        default:
            domain_parser_insert_symbol(&parser);
        }
    }

    if (parser.brackets_count != 0) return -1;

    if (domain->template[0] != '^' && domain->template[length - 1] != '$') {
        domain->prepared_template[0] = '^';
        domain->prepared_template[parser.prepared_pos] = '$';

        parser.prepared_pos++;
    }

    domain_parser_insert_custom_symbol(&parser, 0);

    return 0;
}

int domain_estimate_length(const char* domain) {
    int length = strlen(domain);
    int pcre_length = 0;
    int brackets_count = 0;

    for (int pos = 0; pos < length; pos++) {
        switch (domain[pos]) {
        case '(':
        case '[':
            brackets_count++;
            pcre_length++;
            break;
        case ')':
        case ']':
            if (brackets_count == 0) return -1;
            brackets_count--;
            pcre_length++;
            break;
        case '*':
            if (brackets_count > 0) {
                pcre_length++;
                break;
            }

            if (pos == 0) {
                pcre_length += 2;
            }
            else if (pos == pcre_length - 1) {
                pcre_length += 2;
            }
            else {
                log_error("Domain error: Asterisk must be in start or end of the string\n");
                return -1;
            }
            break;
        case '.':
            if (brackets_count > 0) {
                pcre_length++;
                break;
            }

            pcre_length += 2;
            break;
        default:
            pcre_length++;
        }
    }

    if (brackets_count != 0) return -1;

    if (domain[0] != '^' && domain[length - 1] != '$') {
        pcre_length += 2;
    }

    return pcre_length;
}

void domain_parser_alloc(domain_parser_t* parser, domain_t* domain) {
    parser->template = domain->template;
    parser->prepared_template = domain->prepared_template;
    parser->pos = 0;
    parser->pcre_pos = 0;
    parser->prepared_pos = 0;
    parser->brackets_count = 0;
}

void domain_parser_insert_symbol(domain_parser_t* parser) {
    domain_parser_insert_custom_symbol(parser, parser->template[parser->pos]);
}

void domain_parser_insert_custom_symbol(domain_parser_t* parser, char ch) {
    parser->prepared_template[parser->prepared_pos] = ch;
    parser->pcre_pos++;
    parser->prepared_pos++;
}

int domain_count(domain_t* domain) {
    domain_t* current = domain;

    int count = 0;

    while (current) {
        count++;
        current = current->next;
    }

    return count;
}