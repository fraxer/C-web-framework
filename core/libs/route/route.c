#include <string.h>

#include "log.h"
#include "route.h"

#define ROUTE_EMPTY_PATH "Route error: Empty path\n"
#define ROUTE_OUT_OF_MEMORY "Route error: Out of memory\n"
#define ROUTE_EMPTY_TOKEN "Route error: Empty token in \"%s\"\n"
#define ROUTE_UNOPENED_TOKEN "Route error: Unopened token in \"%s\"\n"
#define ROUTE_UNCLOSED_TOKEN "Route error: Unclosed token in \"%s\"\n"
#define ROUTE_EMPTY_PARAM_NAME "Route error: Empty param name in \"%s\"\n"
#define ROUTE_EMPTY_PARAM_EXPRESSION "Route error: Empty param expression in \"%s\"\n"
#define ROUTE_PARAM_ONE_WORD "Route error: For param need one word in \"%s\"\n"

typedef struct route_parser {
    int is_primitive;
    int params_count;
    unsigned short int dirty_pos;
    unsigned short int pos;
    const char* dirty_location;
    char* location;
    route_param_t* first_param;
    route_param_t* last_param;
} route_parser_t;

route_t* route_init_route();
int route_init_parser(route_parser_t* parser, const char* dirty_location);
int route_parse_location(route_parser_t* parser);
int route_parse_token(route_parser_t* parser);
void route_insert_custom_symbol(route_parser_t* parser, char symbol);
void route_insert_symbol(route_parser_t* parser);
int route_alloc_param(route_parser_t* parser);
int route_fill_param(route_parser_t* parser);
void route_parser_free(route_parser_t* parser);


route_t* route_create(const char* dirty_location) {
    int result = -1;

    route_t* route = route_init_route();

    route_parser_t parser;

    if (route == NULL) goto failed;

    if (route_init_parser(&parser, dirty_location) == -1) goto failed;

    if (route_parse_location(&parser) == -1) goto failed;

    route->location = pcre_compile(parser.location, 0, &route->location_error, &route->location_erroffset, NULL);

    if (route->location_error != NULL) goto failed;

    route->is_primitive = parser.is_primitive;
    route->params_count = parser.params_count;
    route->path = parser.location;
    route->path_length = strlen(parser.location);
    route->param = parser.first_param;

    result = 0;

    failed:

    if (result == -1 && route) {
        free(route);
        route = NULL;
    }

    route_parser_free(&parser);

    return route;
}

route_t* route_init_route() {
    route_t* route = (route_t*)malloc(sizeof(route_t));

    if (route == NULL) {
        log_error(ROUTE_OUT_OF_MEMORY);
        return NULL;
    }

    route->path = NULL;
    route->path_length = 0;
    route->location_error = NULL;

    route->handler[ROUTE_GET] = NULL;
    route->handler[ROUTE_POST] = NULL;
    route->handler[ROUTE_PUT] = NULL;
    route->handler[ROUTE_DELETE] = NULL;
    route->handler[ROUTE_OPTIONS] = NULL;
    route->handler[ROUTE_PATCH] = NULL;
    route->handler[ROUTE_HEAD] = NULL;

    route->location_erroffset = 0;
    route->location = NULL;
    route->is_primitive = 0;
    route->params_count = 0;
    route->param = NULL;
    route->next = NULL;

    return route;
}

int route_init_parser(route_parser_t* parser, const char* dirty_location) {
    parser->is_primitive = 0;
    parser->params_count = 0;
    parser->dirty_pos = 0;
    parser->pos = 0;
    parser->dirty_location = dirty_location;
    parser->location = (char*)malloc(strlen(dirty_location) + 1);
    parser->first_param = NULL;
    parser->last_param = NULL;

    if (parser->location == NULL) {
        log_error(ROUTE_OUT_OF_MEMORY);
        return -1;
    }

    return 0;
}

int route_parse_location(route_parser_t* parser) {
    parser->is_primitive = 1;

    for (; parser->dirty_location[parser->dirty_pos] != 0; parser->dirty_pos++) {
        switch (parser->dirty_location[parser->dirty_pos]) {
        case '{':
            if (route_parse_token(parser) == -1) return -1;
            parser->is_primitive = 0;
            break;
        case '\\':
            switch (parser->dirty_location[parser->dirty_pos + 1]) {
            case '}':
                route_insert_symbol(parser);
                parser->dirty_pos++;
                break;
            }
            route_insert_symbol(parser);
            break;
        case '}':
            log_error(ROUTE_UNOPENED_TOKEN, parser->dirty_location);
            return -1;
            break;
        case '*':
        case '[':
        case ']':
        case '(':
        case ')':
        case '+':
        case '^':
        case '|':
        case '$':
            route_insert_symbol(parser);
            parser->is_primitive = 0;
            break;
        default:
            route_insert_symbol(parser);
        }
    }

    parser->location[parser->pos] = 0;

    return 0;
}

int route_parse_token(route_parser_t* parser) {
    parser->dirty_pos++;

    int separator_found = 0;
    int brakets_count = 0;
    int start = parser->pos;
    int symbol_finded = 0;

    if (route_alloc_param(parser) == -1) return -1;

    for (; parser->dirty_location[parser->dirty_pos] != 0; parser->dirty_pos++) {
        char ch = parser->dirty_location[parser->dirty_pos];
        switch (ch) {
        case '{':
            if (!separator_found) {
                log_error(ROUTE_EMPTY_TOKEN, parser->dirty_location);
                return -1;
            }
            brakets_count++;
            route_insert_symbol(parser);
            break;
        case '}':
            if (!separator_found) {
                log_error(ROUTE_EMPTY_TOKEN, parser->dirty_location);
                return -1;
            }
            if (parser->pos - start == 0) {
                log_error(ROUTE_EMPTY_PARAM_EXPRESSION, parser->dirty_location);
                return -1;
            }
            if (brakets_count == 0) {
                route_insert_custom_symbol(parser, ')');
                return 0;
            }
            brakets_count--;
            route_insert_symbol(parser);
            break;
        case '\\':
            switch (parser->dirty_location[parser->dirty_pos + 1]) {
            case '}':
                log_error(ROUTE_EMPTY_PARAM_EXPRESSION, parser->dirty_location);
                return -1;
                break;
            default:
                if (separator_found) {
                    route_insert_symbol(parser);
                }
            }
            break;
        case '\t':
        case '\r':
        case '\n':
        case ' ':
            if (!separator_found && symbol_finded) {
                switch (parser->dirty_location[parser->dirty_pos + 1]) {
                case '\t':
                case '\r':
                case '\n':
                case ' ':
                case '|':
                    break;
                default:
                    log_error(ROUTE_PARAM_ONE_WORD, parser->dirty_location);
                    return -1;
                }
            }
            break;
        case '|':
            separator_found = 1;
            symbol_finded = 0;
            if (route_fill_param(parser) == -1) return -1;
            parser->pos = start;
            route_insert_custom_symbol(parser, '(');
            break;
        default:
            route_insert_symbol(parser);
            symbol_finded = 1;
        }
    }

    log_error(ROUTE_UNCLOSED_TOKEN, parser->dirty_location);

    return -1;
}

void route_insert_custom_symbol(route_parser_t* parser, char symbol) {
    parser->location[parser->pos] = symbol;
    parser->pos++;
}

void route_insert_symbol(route_parser_t* parser) {
    parser->location[parser->pos] = parser->dirty_location[parser->dirty_pos];
    parser->pos++;
}

int route_alloc_param(route_parser_t* parser) {
    route_param_t* param = (route_param_t*)malloc(sizeof(route_param_t));

    if (param == NULL) {
        log_error(ROUTE_OUT_OF_MEMORY);
        return -1;
    }

    param->start = parser->pos;
    param->end = parser->pos;
    param->string_len = 0;
    param->string = NULL;
    param->next = NULL;

    parser->params_count++;

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

int route_fill_param(route_parser_t* parser) {
    parser->last_param->end = parser->pos;
    parser->last_param->string_len = parser->last_param->end - parser->last_param->start;

    if (parser->last_param->string_len == 0) {
        log_error(ROUTE_EMPTY_PARAM_NAME, parser->dirty_location);
        return -1;
    }

    parser->last_param->string = (char*)malloc(parser->last_param->string_len + 1);

    if (parser->last_param->string == NULL) {
        log_error(ROUTE_OUT_OF_MEMORY);
        return -1;
    }

    strncpy(parser->last_param->string, &parser->location[parser->last_param->start], parser->last_param->string_len);

    parser->last_param->string[parser->last_param->string_len] = 0;

    return 0;
}

void route_parser_free(route_parser_t* parser) {
    parser->location = NULL;
}

int route_set_http_handler(route_t* route, const char* method, void(*function)(void*, void*)) {
    int m = ROUTE_NONE;

    if (method[0] == 'G' && method[1] == 'E' && method[2] == 'T') {
        m = ROUTE_GET;
    }
    else if (method[0] == 'P' && method[1] == 'O' && method[2] == 'S' && method[3] == 'T') {
        m = ROUTE_POST;
    }
    else if (method[0] == 'P' && method[1] == 'U' && method[2] == 'T') {
        m = ROUTE_PUT;
    }
    else if (method[0] == 'D' && method[1] == 'E' && method[2] == 'L' && method[3] == 'E' && method[4] == 'T' && method[5] == 'E') {
        m = ROUTE_DELETE;
    }
    else if (method[0] == 'O' && method[1] == 'P' && method[2] == 'T' && method[3] == 'I' && method[4] == 'O' && method[5] == 'N' && method[6] == 'S') {
        m = ROUTE_OPTIONS;
    }
    else if (method[0] == 'P' && method[1] == 'A' && method[2] == 'T' && method[3] == 'C' && method[4] == 'H') {
        m = ROUTE_PATCH;
    }
    else if (method[0] == 'H' && method[1] == 'E' && method[2] == 'A' && method[3] == 'D') {
        m = ROUTE_HEAD;
    }

    if (m == ROUTE_NONE) return 0;

    if (route->handler[m]) return 1;

    route->handler[m] = function;

    return 1;
}

int route_set_websockets_handler(route_t* route, const char* method, void(*function)(void*, void*)) {
    int m = ROUTE_NONE;

    if (method[0] == 'G' && method[1] == 'E' && method[2] == 'T') {
        m = ROUTE_GET;
    }
    else if (method[0] == 'P' && method[1] == 'O' && method[2] == 'S' && method[3] == 'T') {
        m = ROUTE_POST;
    }
    else if (method[0] == 'D' && method[1] == 'E' && method[2] == 'L' && method[3] == 'E' && method[4] == 'T' && method[5] == 'E') {
        m = ROUTE_DELETE;
    }
    else if (method[0] == 'P' && method[1] == 'A' && method[2] == 'T' && method[3] == 'C' && method[4] == 'H') {
        m = ROUTE_PATCH;
    }

    if (m == ROUTE_NONE) return 0;

    if (route->handler[m]) return 1;

    route->handler[m] = function;

    return 1;
}

void routes_free(route_t* route) {
    while (route != NULL) {
        route_t* route_next = route->next;
        
        route_param_t* param = route->param;
        while (param != NULL) {
            route_param_t* param_next = param->next;

            free(param->string);
            free(param);

            param = param_next;
        }

        if (route->location != NULL)
            pcre_free(route->location);

        free(route->path);
        free(route);

        route = route_next;
    }
}

int route_compare_primitive(route_t* route, const char* path, size_t length) {
    if (route->path_length != length) return 0;

    for (size_t i = 0, j = 0; i < route->path_length && j < length; i++, j++) {
        if (route->path[i] != path[j]) return 0;
    }

    return 1;
}