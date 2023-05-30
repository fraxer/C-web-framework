#include <ctype.h>
#include <string.h>
#include "pcre.h"
#include "redirect.h"
#include "../log/log.h"
    #include <stdio.h>

#define REDIRECT_OUT_OF_MEMORY "Redirect error: Out of memory\n"
#define REDIRECT_BIG_VALUE_PARAM "Redirect error: Big number in param \"%s\"\n"
#define REDIRECT_ERROR_VALUE_PARAM "Redirect error: param is not number \"%s\"\n"
#define REDIRECT_ERROR_CHECK_PARAM "Redirect error: params count is not equal substrings count in location \"%s\"\n"

typedef struct redirect_parser {
    int params_count;
    size_t start_pos;
    size_t pos;
    const char* string;
    redirect_param_t* first_param;
    redirect_param_t* last_param;
} redirect_parser_t;

redirect_t* redirect_init();
int redirect_init_parser(redirect_parser_t*, const char*);
int redirect_parse_destination(redirect_parser_t*);
int redirect_check_params(redirect_t*);
int redirect_parse_token(redirect_parser_t*);
int redirect_alloc_param(redirect_parser_t*, size_t, size_t, int);
int redirect_fill_param(redirect_parser_t*);
void redirect_parser_free(redirect_parser_t*);
void redirect_append_uri(char*, size_t*, const char*, size_t);


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

    redirect->params_count = parser.params_count;
    redirect->param = parser.first_param;

    if (redirect_check_params(redirect) == -1) goto failed;

    result = redirect;

    failed:

    if (result == NULL) {
        redirect_free(redirect);
    }

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

    redirect->params_count = 0;
    redirect->location_erroffset = 0;
    redirect->location = NULL;
    redirect->param = NULL;
    redirect->next = NULL;

    if (redirect->template == NULL) {
        log_error(REDIRECT_OUT_OF_MEMORY);
        free(redirect);
        return NULL;
    }

    strcpy(redirect->template, template);

    return redirect;
}

int redirect_init_parser(redirect_parser_t* parser, const char* string) {
    parser->params_count = 0;
    parser->start_pos = 0;
    parser->pos = 0;
    parser->string = string;
    parser->first_param = NULL;
    parser->last_param = NULL;

    return 0;
}

int redirect_parse_destination(redirect_parser_t* parser) {
    if (strlen(parser->string) == 0) return -1;

    for (parser->pos = 0; parser->string[parser->pos] != 0; parser->pos++) {
        switch (parser->string[parser->pos]) {
        case '{':
            if (redirect_parse_token(parser) == -1) return -1;
            break;
        }
    }

    return 0;
}

int redirect_check_params(redirect_t* redirect) {
    int where = 0;

    if (pcre_fullinfo(redirect->location, NULL, PCRE_INFO_CAPTURECOUNT, &where) != 0) return -1;

    if (where != redirect->params_count) {
        log_error(REDIRECT_ERROR_CHECK_PARAM, redirect->template);
        return -1;
    }

    return 0;
}

int redirect_parse_token(redirect_parser_t* parser) {
    parser->start_pos = parser->pos;
    parser->pos++;

    for (; parser->string[parser->pos] != 0; parser->pos++) {
        char c = parser->string[parser->pos];

        if (c == '}') {
            int result = redirect_fill_param(parser);
            if (result == -1) return -1;
            if (result == -2) parser->pos++;
            break;
        }
        if (c == '{' || !isdigit(c)) {
            parser->pos--;
            return 0;
        }
    }

    return 0;
}

int redirect_alloc_param(redirect_parser_t* parser, size_t start, size_t end, int number) {
    redirect_param_t* param = (redirect_param_t*)malloc(sizeof(redirect_param_t));

    if (param == NULL) {
        log_error(REDIRECT_OUT_OF_MEMORY);
        return -1;
    }

    param->start = start;
    param->end = end;
    param->number = number;
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
    size_t start = parser->start_pos;
    size_t end = parser->pos + 1;

    size_t string_number_length = (end - 1) - (start + 1);
    size_t max_number_length = 2;

    if (string_number_length == 0) return -2;

    if (string_number_length > max_number_length) {
        log_error(REDIRECT_BIG_VALUE_PARAM, &parser->string[start]);
        return -1;
    }

    char string_number[4] = {0,0,0,0};

    strncpy(string_number, &parser->string[start + 1], string_number_length);

    int number = atoi(string_number);
    if (number < 0) {
        log_error(REDIRECT_ERROR_VALUE_PARAM, &parser->string[start]);
        return -1;
    }

    if (redirect_alloc_param(parser, start, end, number) == -1) return -1;

    parser->params_count++;

    return 0;
}

void redirect_parser_free(redirect_parser_t* parser) {
    parser->string = NULL;
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

char* redirect_get_uri(redirect_t* redirect, const char* string, int* vector) {
    char* uri = NULL;

    size_t uri_length = 0;

    if (redirect->param) {
        size_t start_pos = 0;

        redirect_param_t* param = redirect->param;
        redirect_param_t* last_param = redirect->param;

        for (param = redirect->param; param; param = param->next) {
            size_t param_string_length = vector[param->number * 2 + 1] - vector[param->number * 2];
            size_t substring_length = param->start - start_pos;

            uri_length += substring_length;
            uri_length += param_string_length;
        }

        uri = (char*)malloc(uri_length + 1);

        if (uri == NULL) return NULL;

        uri_length = 0;

        for (param = redirect->param; param; param = param->next) {
            size_t param_string_length = vector[param->number * 2 + 1] - vector[param->number * 2];

            size_t substring_length = param->start - start_pos;

            redirect_append_uri(uri, &uri_length, &redirect->template[start_pos], substring_length);

            redirect_append_uri(uri, &uri_length, &string[vector[param->number * 2]], param_string_length);

            start_pos = param->end;

            last_param = param;
        }

        if (last_param->end < redirect->template_length) {
            size_t substring_length = redirect->template_length - last_param->end;

            redirect_append_uri(uri, &uri_length, &redirect->template[last_param->end], substring_length);
        }
    }
    else {
        uri = (char*)malloc(redirect->template_length + 1);

        if (uri == NULL) return NULL;

        redirect_append_uri(uri, &uri_length, redirect->template, redirect->template_length);
    }

    return uri;
}

void redirect_append_uri(char* uri, size_t* offset, const char* string, size_t length) {
    strncpy(&uri[*offset], string, length);

    *offset += length;

    uri[*offset] = 0;
}