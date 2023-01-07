#include <ctype.h>
#include <string.h>
#include "pcre.h"
#include "redirect.h"
#include "../log/log.h"
    #include <stdio.h>

#define REDIRECT_EMPTY_PATH "Redirect error: Empty path\n"
#define REDIRECT_OUT_OF_MEMORY "Redirect error: Out of memory\n"
#define REDIRECT_EMPTY_TOKEN "Redirect error: Empty token in \"%s\"\n"
#define REDIRECT_UNOPENED_TOKEN "Redirect error: Unopened token in \"%s\"\n"
#define REDIRECT_UNCLOSED_TOKEN "Redirect error: Unclosed token in \"%s\"\n"
#define REDIRECT_EMPTY_PARAM_NAME "Redirect error: Empty param name in \"%s\"\n"
#define REDIRECT_EMPTY_PARAM_EXPRESSION "Redirect error: Empty param expression in \"%s\"\n"
#define REDIRECT_PARAM_ONE_WORD "Redirect error: For param need one word in \"%s\"\n"

typedef struct redirect_parser {
    size_t pos;
    const char* string;
    redirect_param_t* first_param;
    redirect_param_t* last_param;
} redirect_parser_t;

redirect_t* redirect_init();
int redirect_init_parser(redirect_parser_t*, const char*);
int redirect_parse_destination(redirect_parser_t*);
int redirect_parse_token(redirect_parser_t*);
int redirect_alloc_param(redirect_parser_t*);
int redirect_fill_param(redirect_parser_t*);
void redirect_parser_free(redirect_parser_t*);


redirect_t* redirect_create(const char* location, const char* destination) {
    redirect_t* result = NULL;

    redirect_t* redirect = redirect_init(destination);

    redirect_parser_t parser;

    if (redirect == NULL) goto failed;

    if (redirect_init_parser(&parser, destination) == -1) goto failed;

    if (redirect_parse_destination(&parser) == -1) goto failed;

    redirect->location = pcre_compile(location, 0, &redirect->location_error, &redirect->location_erroffset, NULL);

    if (redirect->location == NULL) goto failed;
    if (redirect->location_error != NULL) goto failed;

    redirect->param = parser.first_param;

    // if (redirect_check_params(redirect) == -1) goto failed;

    result = redirect;

    failed:

    redirect_parser_free(&parser);

    return result;
}

redirect_t* redirect_init(const char* template) {
    redirect_t* redirect = (redirect_t*)malloc(sizeof(redirect_t));

    if (redirect == NULL) {
        log_error(REDIRECT_OUT_OF_MEMORY);
        return NULL;
    }

    redirect->template = (char*)malloc(strlen(template) + 1);
    redirect->template_length = strlen(template);
    redirect->location_error = NULL;

    redirect->location_erroffset = 0;
    redirect->location = NULL;
    redirect->param = NULL;
    redirect->next = NULL;

    if (redirect->template == NULL) {
        log_error(REDIRECT_OUT_OF_MEMORY);
        return NULL;
    }

    strcpy(redirect->template, template);

    return redirect;
}

int redirect_init_parser(redirect_parser_t* parser, const char* string) {
    parser->pos = 0;
    parser->string = string;
    parser->first_param = NULL;
    parser->last_param = NULL;

    return 0;
}

int redirect_parse_destination(redirect_parser_t* parser) {
    for (parser->pos = 0; parser->string[parser->pos] != 0; parser->pos++) {
        switch (parser->string[parser->pos]) {
        case '#':
            if (redirect_parse_token(parser) == -1) return -1;
            break;
        }
    }

    return 0;
}

int redirect_parse_token(redirect_parser_t* parser) {
    parser->pos++;

    size_t start = parser->pos;

    if (redirect_alloc_param(parser) == -1) return -1;

    for (; parser->string[parser->pos] != 0; parser->pos++) {
        char c = parser->string[parser->pos];

        if (!isdigit(c)) {
            if (redirect_fill_param(parser) == -1) return -1;
            break;
        }
    }

    if (redirect_fill_param(parser) == -1) return -1;

    return 0;
}

int redirect_alloc_param(redirect_parser_t* parser) {
    redirect_param_t* param = (redirect_param_t*)malloc(sizeof(redirect_param_t));

    if (param == NULL) {
        log_error(REDIRECT_OUT_OF_MEMORY);
        return -1;
    }

    param->start = parser->pos;
    param->end = parser->pos;
    param->next = NULL;

    if (!parser->first_param) {
        parser->first_param = param;
    }

    if (!parser->last_param) {
        parser->last_param = param;
    } else {
        parser->last_param->next = param;
        parser->last_param = param;
    }

    return 0;
}

int redirect_fill_param(redirect_parser_t* parser) {
    parser->last_param->end = parser->pos;

    if (parser->last_param->end - parser->last_param->start == 0) {
        log_error(REDIRECT_EMPTY_PARAM_NAME, parser->string);
        return -1;
    }

    // printf("param: %d, %d, %d, %s\n", parser->last_param->start, parser->last_param->end, parser->last_param->string_len, parser->last_param->string);

    return 0;
}

void redirect_parser_free(redirect_parser_t* parser) {
    parser->string = NULL;
}

int redirect_set_http_handler(redirect_t* redirect, const char* method, void* function) {
    // int m = REDIRECT_NONE;

    // if (method[0] == 'G' && method[1] == 'E' && method[2] == 'T') {
    //     m = REDIRECT_GET;
    // }
    // else if (method[0] == 'P' && method[1] == 'O' && method[2] == 'S' && method[3] == 'T') {
    //     m = REDIRECT_POST;
    // }
    // else if (method[0] == 'P' && method[1] == 'U' && method[2] == 'T') {
    //     m = REDIRECT_PUT;
    // }
    // else if (method[0] == 'D' && method[1] == 'E' && method[2] == 'L' && method[3] == 'E' && method[4] == 'T' && method[5] == 'E') {
    //     m = REDIRECT_DELETE;
    // }
    // else if (method[0] == 'O' && method[1] == 'P' && method[2] == 'T' && method[3] == 'I' && method[4] == 'O' && method[5] == 'N' && method[6] == 'S') {
    //     m = REDIRECT_OPTIONS;
    // }
    // else if (method[0] == 'P' && method[1] == 'A' && method[2] == 'T' && method[3] == 'C' && method[4] == 'H') {
    //     m = REDIRECT_PATCH;
    // }

    // if (m == REDIRECT_NONE) return -1;

    // if (redirect->handler[m]) return 0;

    // redirect->handler[m] = (void(*)(request_t*, response_t*))function;

    return 0;
}

void redirect_free(redirect_t* redirect) {
    while (redirect) {
        redirect_t* redirect_next = redirect->next;

        redirect_param_t* param = redirect->param;

        while (param) {
            redirect_param_t* param_next = param->next;

            free(param);

            param = param_next;
        }

        if (redirect->location) pcre_free(redirect->location);

        if (redirect->template) free(redirect->template);
        free(redirect);

        redirect = redirect_next;
    }
}