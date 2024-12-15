#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "str.h"
#include "helpers.h"
#include "http1common.h"

http1_header_t* http1_header_alloc();
http1_query_t* http1_query_alloc();
http1_cookie_t* http1_cookie_alloc();


http1_header_t* http1_header_alloc() {
    return malloc(sizeof(http1_header_t));
}

http1_header_t* http1_header_create(const char* key, size_t key_length, const char* value, size_t value_length) {
    http1_header_t* header = http1_header_alloc();

    if (header == NULL) return NULL;

    header->key = NULL;
    header->key_length = key_length;
    header->value = NULL;
    header->value_length = value_length;
    header->next = NULL;

    if (key_length)
        header->key = http1_set_field(key, key_length);

    if (value_length)
        header->value = http1_set_field(value, value_length);

    return header;
}

void http1_header_free(http1_header_t* header) {
    if (header->key)
        free((void*)header->key);

    if (header->value)
        free((void*)header->value);

    free(header);
}

void http1_headers_free(http1_header_t* header) {
    while (header) {
        http1_header_t* next = header->next;
        http1_header_free(header);
        header = next;
    }
}

http1_header_t* http1_header_delete(http1_header_t* header, const char* key) {
    if (header == NULL) return NULL;

    if (cmpstrn_lower(header->key, header->key_length, key, strlen(key))) {
        http1_header_t* next = header->next;
        http1_header_free(header);
        return next;
    }

    http1_header_t* first_header = header;
    http1_header_t* prev_header = header;

    header = header->next;

    while (header) {
        http1_header_t* next = header->next;

        if (cmpstrn_lower(header->key, header->key_length, key, strlen(key))) {
            prev_header->next = next;
            http1_header_free(header);
            break;
        }

        prev_header = header;
        header = next;
    }

    return first_header;
}

http1_query_t* http1_query_alloc() {
    return malloc(sizeof(http1_query_t));
}

http1_query_t* http1_query_create(const char* key, size_t key_length, const char* value, size_t value_length) {
    http1_query_t* query = http1_query_alloc();

    if (query == NULL) return NULL;

    query->key = NULL;
    query->value = NULL;
    query->next = NULL;

    if (key && key_length)
        query->key = http1_set_field(key, key_length);

    if (value && value_length)
        query->value = http1_set_field(value, value_length);

    return query;
}

void http1_query_free(http1_query_t* query) {
    free((void*)query->key);
    free((void*)query->value);
    free(query);
}

void http1_queries_free(http1_query_t* query) {
    while (query != NULL) {
        http1_query_t* next = query->next;
        http1_query_free(query);
        query = next;
    }
}

char* http1_query_str(http1_query_t* query) {
    if (query == NULL) return NULL;

    str_t* uri = str_create_empty();
    if (uri == NULL) return NULL;

    while (query != NULL) {
        str_append(uri, query->key, strlen(query->key));
        str_appendc(uri, '=');
        str_append(uri, query->value, strlen(query->value));
        if (query->next != NULL)
            str_appendc(uri, '&');

        query = query->next;
    }

    char* string = str_copy(uri);
    str_free(uri);

    return string;
}

char* http1_set_field(const char* string, size_t length) {
    char* value = malloc(length + 1);
    if (value == NULL) return value;

    if (string != NULL) {
        memcpy(value, string, length);
        value[length] = 0;
    }

    return value;
}

http1_payloadpart_t* http1_payloadpart_create() {
    http1_payloadpart_t* part = malloc(sizeof * part);
    if (part == NULL) return NULL;

    part->field = NULL;
    part->next = NULL;
    part->header = NULL;
    part->offset = 0;
    part->size = 0;

    return part;
}

void http1_payloadpart_free(http1_payloadpart_t* part) {
    while (part) {
        http1_payloadpart_t* next = part->next;

        http1_payloadfield_free(part->field);

        http1_header_t* header = part->header;
        while (header) {
            http1_header_t* next = header->next;
            http1_header_free(header);
            header = next;
        }

        free(part);

        part = next;
    }
}

http1_payloadfield_t* http1_payloadfield_create() {
    http1_payloadfield_t* field = malloc(sizeof * field);
    if (field == NULL) return NULL;
    field->key = NULL;
    field->key_length = 0;
    field->value = NULL;
    field->value_length = 0;
    field->next = NULL;
    return field;
}

void http1_payloadfield_free(http1_payloadfield_t* field) {
    while (field) {
        http1_payloadfield_t* next = field->next;

        if (field->key) free(field->key);
        if (field->value) free(field->value);
        free(field);

        field = next;
    }
}

http1_cookie_t* http1_cookie_alloc() {
    return malloc(sizeof(http1_cookie_t));
}

http1_cookie_t* http1_cookie_create() {
    http1_cookie_t* cookie = http1_cookie_alloc();

    if (cookie == NULL) return NULL;

    cookie->key = NULL;
    cookie->key_length = 0;
    cookie->value = NULL;
    cookie->value_length = 0;
    cookie->next = NULL;

    return cookie;
}

void http1_cookie_free(http1_cookie_t* cookie) {
    while (cookie) {
        http1_cookie_t* next = cookie->next;

        if (cookie->key) free(cookie->key);
        if (cookie->value) free(cookie->value);
        if (cookie) free(cookie);

        cookie = next;
    }
}
