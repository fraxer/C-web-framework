#include "cookieparser.h"

void cookieparser_init(cookieparser_t* parser) {
    parser->payload_offset = 0;
    parser->offset = 0;
    parser->size = 0;
    parser->cookie = NULL;
    parser->last_cookie = NULL;
}

void cookieparser_parse(cookieparser_t* parser, char* buffer, size_t buffer_size) {
    enum { KEY, VALUE, SPACE } stage = KEY;

    http1_cookie_t* cookie = http1_cookie_create();
    if (cookie == NULL) return;

    parser->cookie = cookie;
    parser->last_cookie = cookie;

    for (size_t i = 0; i <= buffer_size; i++) {
        char ch = buffer[i];

        switch (stage) {
        case KEY:
            if (ch == '=') {
                cookie->key = http1_set_field(&buffer[parser->offset], parser->size);
                cookie->key_length = parser->size;

                parser->offset = parser->payload_offset + 1;
                parser->size = -1;

                if (cookie->key == NULL) return;

                stage = VALUE;
            }
            break;
        case VALUE:
            if (ch == ';' || ch == '\0') {
                cookie->value = http1_set_field(&buffer[parser->offset], parser->size);
                cookie->value_length = parser->size;

                parser->offset = parser->payload_offset + 1;
                parser->size = 0;

                if (cookie->value == NULL) return;

                if (ch == '\0') return;

                http1_cookie_t* cookie_new = http1_cookie_create();

                if (cookie_new == NULL) return;

                if (parser->last_cookie)
                    parser->last_cookie->next = cookie_new;
                parser->last_cookie = cookie_new;

                cookie = cookie_new;

                stage = SPACE;
            }
            break;
        case SPACE:
            if (ch != ' ') {
                parser->offset = parser->payload_offset;
                parser->size = 0;

                stage = KEY;
            }
            break;
        }

        parser->payload_offset++;
        parser->size++;
    }
}

http1_cookie_t* cookieparser_cookie(cookieparser_t* parser) {
    return parser->cookie;
}