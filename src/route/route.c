#include <string.h>
    #include <stdio.h>
#include "pcre.h"
#include "route.h"
#include "../log/log.h"

typedef struct route_parser {
    const char* dirty_location;
    unsigned short int dirty_pos;
    char* location;
    unsigned short int pos;
    route_param_t* first_param;
    route_param_t* last_param;
} route_parser_t;

static route_t* first_route = NULL;
static route_t* last_route = NULL;

route_t* route_init_route();

int route_init_parser(route_parser_t* parser, const char* dirty_location);

int route_parse_location(route_parser_t* parser, const char* dirty_location);

int route_parse_token(route_parser_t* parser);

void route_insert_symbol(route_parser_t* parser);

int route_alloc_param(route_parser_t* parser);

int route_fill_param(route_parser_t* parser);

void route_parser_free(route_parser_t* parser);

void* route_init() {
    return 0;
}

route_t* route_create(const char* dirty_location, const char* redirect_location) {
    int result = -1;

    route_t* route = route_init_route();

    route_parser_t parser;

    if (route == NULL) goto failed;

    if (route_init_parser(&parser, dirty_location) == -1) goto failed;

    if (route_parse_location(&parser, dirty_location) == -1) goto failed;

    route->location = pcre_compile(parser.location, 0, &route->location_error, &route->location_erroffset, NULL);
    route->dest = redirect_location;

    if (route->location_error != NULL) goto failed;

    if (first_route == NULL) {
        first_route = route;
    }

    if (last_route) {
        last_route->next = route;
    }

    last_route = route;

    route->path = parser.location;
    route->param = parser.first_param;

    result = 0;

    failed:

    if (result == -1 && route) free(route);

    route_parser_free(&parser);

    return route;
}

route_t* route_get_first_route() {
    return first_route;
}

route_t* route_init_route() {
    route_t* route = (route_t*)malloc(sizeof(route_t));

    if (route == NULL) {
        log_error(ROUTE_OUT_OF_MEMORY);
        return NULL;
    }

    route->path = NULL;
    route->dest = NULL;
    route->location_error = NULL;

    route->method[ROUTE_GET] = NULL;
    route->method[ROUTE_POST] = NULL;
    route->method[ROUTE_PUT] = NULL;
    route->method[ROUTE_DELETE] = NULL;
    route->method[ROUTE_OPTIONS] = NULL;
    route->method[ROUTE_PATCH] = NULL;

    route->location_erroffset = 0;
    route->location = NULL;
    route->param = NULL;
    route->next = NULL;

    return route;
}

int route_init_parser(route_parser_t* parser, const char* dirty_location) {
    parser->dirty_location = dirty_location;
    parser->location = (char*)malloc(strlen(dirty_location) + 1);
    parser->dirty_pos = 0;
    parser->pos = 0;
    parser->first_param = NULL;
    parser->last_param = NULL;

    if (parser->location == NULL) {
        log_error(ROUTE_OUT_OF_MEMORY);
        return -1;
    }

    return 0;
}

int route_parse_location(route_parser_t* parser, const char* dirty_location) {
    for (; parser->dirty_location[parser->dirty_pos] != 0; parser->dirty_pos++) {
        switch (parser->dirty_location[parser->dirty_pos]) {
        case '{':
            if (route_parse_token(parser) == -1) return -1;
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
        default:
            route_insert_symbol(parser);
        }
    }

    parser->location[parser->pos] = 0;

    // printf("route: %s\n", parser->location);

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
        switch (parser->dirty_location[parser->dirty_pos]) {
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
            if (brakets_count == 0) return 0;
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
            break;
        default:
            route_insert_symbol(parser);
            symbol_finded = 1;
        }
    }

    log_error(ROUTE_UNCLOSED_TOKEN, parser->dirty_location);

    return -1;
}

void route_insert_symbol(route_parser_t* parser) {
    parser->location[parser->pos] = parser->dirty_location[parser->dirty_pos];

    // printf("%d, %d, %.*s\n", parser->pos, parser->location[parser->pos], parser->pos, parser->location);

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

    // printf("param: %d, %d, %d, %s\n", parser->last_param->start, parser->last_param->end, parser->last_param->string_len, parser->last_param->string);

    return 0;
}

void route_parser_free(route_parser_t* parser) {
    parser->location = NULL;
}

int route_set_method_handler(route_t* route, const char* method, void* function) {
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

    if (m == ROUTE_NONE) return -1;

    if (route->method[m]) return 0;

    route->method[m] = (void*(*)(void*))function;

    return 0;
}

void route_free(route_t* route) {
    while (route) {
        route_t* route_next = route->next;

        route_param_t* param = route->param;

        while (param) {
            route_param_t* param_next = param->next;

            free(param->string);
            free(param);

            param = param_next;
        }

        free(route->path);
        free(route);

        route = route_next;
    }
}

void route_reset_internal() {
    first_route = NULL;
    last_route = NULL;
}