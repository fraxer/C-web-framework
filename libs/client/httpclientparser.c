#include <stdlib.h>
#include <string.h>

#include "http1common.h"
#include "httpclient.h"
#include "httpclientparser.h"

void __httpclientparser_flush(httpclientparser_t* parser);
int __httpclientparser_set_query(httpclientparser_t* parser, const char* string, size_t length, size_t pos);
void __httpclientparser_append_query(httpclientparser_t* parser, http1_query_t* query);
int __httpclientparser_set_path(httpclientparser_t* parser, const char* string, size_t length);
int __httpclientparser_set_ext(httpclientparser_t* parser, const char* string, size_t length);
int __httpclientparser_set_protocol(httpclientparser_t* parser);
int __httpclientparser_set_host(httpclientparser_t* parser);
int __httpclientparser_set_port(httpclientparser_t* parser);
int __httpclientparser_set_uri(httpclientparser_t* parser);

void httpclientparser_init(httpclientparser_t* parser) {
    parser->stage = CLIENTPARSER_PROTOCOL;
    parser->bytes_readed = 0;
    parser->pos_start = 0;
    parser->pos = 0;
    parser->use_ssl = 0;
    parser->port = 0;
    parser->host = NULL;
    parser->uri = NULL;
    parser->path = NULL;
    parser->ext = NULL;
    parser->buffer = NULL;
    parser->query = NULL;
    parser->last_query = NULL;

    bufferdata_init(&parser->buf);
}

int httpclientparser_move_use_ssl(httpclientparser_t* parser) {
    return parser->use_ssl;
}

char* httpclientparser_move_host(httpclientparser_t* parser) {
    char* host = parser->host;
    parser->host = NULL;

    return host;
}

short int httpclientparser_move_port(httpclientparser_t* parser) {
    return parser->port;
}

char* httpclientparser_move_uri(httpclientparser_t* parser) {
    char* uri = parser->uri;
    parser->uri = NULL;

    return uri;
}

char* httpclientparser_move_path(httpclientparser_t* parser) {
    char* path = parser->path;
    parser->path = NULL;

    return path;
}

char* httpclientparser_move_ext(httpclientparser_t* parser) {
    char* ext = parser->ext;
    parser->ext = NULL;

    return ext;
}

http1_query_t* httpclientparser_move_query(httpclientparser_t* parser) {
    http1_query_t* query = parser->query;
    parser->query = NULL;

    return query;
}

http1_query_t* httpclientparser_move_last_query(httpclientparser_t* parser) {
    http1_query_t* last_query = parser->last_query;
    parser->last_query = NULL;

    return last_query;
}


void httpclientparser_free(httpclientparser_t* parser) {
    __httpclientparser_flush(parser);
    free(parser);
}

void httpclientparser_reset(httpclientparser_t* parser) {
    __httpclientparser_flush(parser);
    httpclientparser_init(parser);
}

int httpclientparser_parse(httpclientparser_t* parser, const char* url) {
    parser->buffer = (char*)url;
    parser->bytes_readed = strlen(url);
    parser->pos_start = 0;
    parser->pos = 0;

    for (parser->pos = parser->pos_start; parser->pos < parser->bytes_readed; parser->pos++) {
        char ch = parser->buffer[parser->pos];
        int end = parser->pos + 1 == parser->bytes_readed;

        switch (parser->stage)
        {
        case CLIENTPARSER_PROTOCOL:
            if (ch == '/') {
                if (bufferdata_writed(&parser->buf) > 0)
                    return CLIENTPARSER_BAD_PROTOCOL;

                parser->stage = CLIENTPARSER_URI;

                if (end || ch == '/')
                    parser->pos--;
                break;
            }
            else if (ch == ':') {
                parser->stage = CLIENTPARSER_PROTOCOL_SLASH1;

                bufferdata_complete(&parser->buf);
                if (!__httpclientparser_set_protocol(parser))
                    return CLIENTPARSER_BAD_PROTOCOL;

                bufferdata_reset(&parser->buf);
                break;
            }
            else if (end) 
                return CLIENTPARSER_BAD_PROTOCOL;
            else {
                if (bufferdata_writed(&parser->buf) > 5)
                    return CLIENTPARSER_BAD_PROTOCOL;

                bufferdata_push(&parser->buf, ch);
                break;
            }
        case CLIENTPARSER_PROTOCOL_SLASH1:
            if (ch == '/') {
                parser->stage = CLIENTPARSER_PROTOCOL_SLASH2;
                break;
            }
            else if (end) 
                return CLIENTPARSER_BAD_PROTOCOL_SEPARATOR;
            else
                return CLIENTPARSER_BAD_PROTOCOL_SEPARATOR;
        case CLIENTPARSER_PROTOCOL_SLASH2:
            if (ch == '/') {
                parser->stage = CLIENTPARSER_HOST;
                break;
            }
            else if (end) 
                return CLIENTPARSER_BAD_PROTOCOL_SEPARATOR;
            else
                return CLIENTPARSER_BAD_PROTOCOL_SEPARATOR;
        case CLIENTPARSER_HOST:
        {
            if (ch == ':')
                parser->stage = CLIENTPARSER_PORT;
            else if (ch == '/')
                parser->stage = CLIENTPARSER_URI;
            else if (end) {
                parser->stage = CLIENTPARSER_URI;
                bufferdata_push(&parser->buf, ch);
            }
            else {
                bufferdata_push(&parser->buf, ch);
                break;
            }

            bufferdata_complete(&parser->buf);
            if (!__httpclientparser_set_host(parser))
                return CLIENTPARSER_BAD_HOST;

            bufferdata_reset(&parser->buf);

            if (ch == '/') {
                parser->pos--;
                break;
            }
            if (end) {
                ch = '/';
                goto start_uri;
            }
            break;
        }
        case CLIENTPARSER_PORT:
        {
            if (ch == '/')
                parser->stage = CLIENTPARSER_URI;
            else if (end) {
                parser->stage = CLIENTPARSER_URI;
                bufferdata_push(&parser->buf, ch);
            }
            else {
                bufferdata_push(&parser->buf, ch);
                break;
            }

            bufferdata_complete(&parser->buf);
            if (!__httpclientparser_set_port(parser))
                return CLIENTPARSER_BAD_PORT;

            bufferdata_reset(&parser->buf);

            if (ch == '/') {
                parser->pos--;
                break;
            }
            if (end) {
                ch = '/';
                goto start_uri;
            }
            break;
        }
        case CLIENTPARSER_URI:
        {
            start_uri:

            bufferdata_push(&parser->buf, ch);
            
            if (!end) break;

            bufferdata_complete(&parser->buf);
            if (!__httpclientparser_set_uri(parser))
                return CLIENTPARSER_BAD_URI;

            bufferdata_reset(&parser->buf);
            break;
        }
        }
    }

    return CLIENTPARSER_OK;
}

void __httpclientparser_flush(httpclientparser_t* parser) {
    if (parser->buf.dynamic_buffer) free(parser->buf.dynamic_buffer);
    parser->buf.dynamic_buffer = NULL;

    bufferdata_reset(&parser->buf);

    if (parser->host) free(parser->host);
    parser->host = NULL;

    if (parser->uri) free(parser->uri);
    parser->uri = NULL;

    if (parser->path) free(parser->path);
    parser->path = NULL;

    if (parser->ext) free(parser->ext);
    parser->ext = NULL;

    http1_queries_free(parser->query);
    parser->query = NULL;
    parser->last_query = NULL;
}

int __httpclientparser_set_protocol(httpclientparser_t* parser) {
    const char* protocol = bufferdata_get(&parser->buf);

    if (protocol[0] == 'h' && protocol[1] == 't' && protocol[2] == 't' && protocol[3] == 'p' && protocol[4] == 's') {
        parser->use_ssl = 1;
        parser->port = 443;
        return 1;
    }
    else if (protocol[0] == 'h' && protocol[1] == 't' && protocol[2] == 't' && protocol[3] == 'p') {
        parser->port = 80;
        return 1;
    }

    return 0;
}

int __httpclientparser_set_host(httpclientparser_t* parser) {
    const size_t length = bufferdata_writed(&parser->buf);
    parser->host = malloc(sizeof(char) * (length + 1));
    if (parser->host == NULL) return 0;

    memcpy(parser->host, bufferdata_get(&parser->buf), length);
    parser->host[length] = 0;

    return 1;
}

int __httpclientparser_set_port(httpclientparser_t* parser) {
    const char* port = bufferdata_get(&parser->buf);

    if (strspn(port, "0123456789") != bufferdata_writed(&parser->buf))
        return 0;

    int port_number = atoi(port);
    if (port_number == 0 || port_number > 65535)
        return 0;

    parser->port = port_number;

    return 1;
}

int __httpclientparser_set_uri(httpclientparser_t* parser) {
    size_t length = bufferdata_writed(&parser->buf);
    const char* string = bufferdata_get(&parser->buf);

    if (length == 0) {
        string = "/";
        length = 1;
    }

    http1_urlendec_t st = http1_urldecode(string, length);
    if (st.string == NULL) return 0;

    parser->uri = st.string;
    string = st.string;
    length = st.length;

    size_t path_point_end = 0;
    size_t ext_point_start = 0;

    for (size_t pos = 0; pos < length; pos++) {
        switch (string[pos]) {
        case '?':
            if (path_point_end == 0) path_point_end = pos;

            if (!__httpclientparser_set_query(parser, string, length, pos))
                return 0;

            goto next;
        case '#':
            if (path_point_end == 0) path_point_end = pos;
            goto next;
        }
    }

    next:

    if (path_point_end == 0) path_point_end = length;
    if (!__httpclientparser_set_path(parser, string, path_point_end))
        return 0;

    if (ext_point_start == 0) ext_point_start = path_point_end;
    if (!__httpclientparser_set_ext(parser, &string[ext_point_start], path_point_end - ext_point_start))
        return 0;

    return 1;
}

int __httpclientparser_set_query(httpclientparser_t* parser, const char* string, size_t length, size_t pos) {
    size_t point_start = pos + 1;
    enum { KEY, VALUE } stage = KEY;

    http1_query_t* query = http1_query_create(NULL, 0, NULL, 0);
    if (query == NULL) return 0;

    parser->query = query;
    parser->last_query = query;

    for (++pos; pos < length; pos++) {
        switch (string[pos]) {
        case '=':
            if (string[pos - 1] == '=') continue;

            stage = VALUE;

            query->key = http1_set_field(&string[point_start], pos - point_start);
            if (query->key == NULL) return 0;

            point_start = pos + 1;
            break;
        case '&':
            stage = KEY;

            query->value = http1_set_field(&string[point_start], pos - point_start);
            if (query->value == NULL) return 0;

            http1_query_t* query_new = http1_query_create(NULL, 0, NULL, 0);
            if (query_new == NULL) return 0;

            __httpclientparser_append_query(parser, query_new);

            query = query_new;

            point_start = pos + 1;
            break;
        case '#':
            if (stage == KEY) {
                query->key = http1_set_field(&string[point_start], pos - point_start);
                if (query->key == NULL) return 0;
            }
            else if (stage == VALUE) {
                query->value = http1_set_field(&string[point_start], pos - point_start);
                if (query->value == NULL) return 0;
            }

            return 1;
        }
    }

    if (stage == KEY) {
        query->key = http1_set_field(&string[point_start], pos - point_start);
        if (query->key == NULL) return 0;

        query->value = http1_set_field("", 1);
        if (query->value == NULL) return 0;
    }
    else if (stage == VALUE) {
        query->value = http1_set_field(&string[point_start], pos - point_start);
        if (query->value == NULL) return 0;
    }

    return 1;
}

void __httpclientparser_append_query(httpclientparser_t* parser, http1_query_t* query) {
    if (parser->query == NULL)
        parser->query = query;

    if (parser->last_query)
        parser->last_query->next = query;

    parser->last_query = query;
}

int __httpclientparser_set_path(httpclientparser_t* parser, const char* string, size_t length) {
    parser->path = malloc(sizeof(char) * (length + 1));
    if (parser->path == NULL) return 0;

    memcpy(parser->path, string, length);
    parser->path[length] = 0;

    return 1;
}

int __httpclientparser_set_ext(httpclientparser_t* parser, const char* string, size_t length) {
    parser->ext = malloc(sizeof(char) * (length + 1));
    if (parser->ext == NULL) return 0;

    memcpy(parser->ext, string, length);
    parser->ext[length] = 0;

    return 1;
}