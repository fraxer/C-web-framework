#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "http1common.h"

http1_header_t* http1_header_alloc();
http1_query_t* http1_query_alloc();


http1_header_t* http1_header_alloc() {
    return (http1_header_t*)malloc(sizeof(http1_header_t));
}

http1_header_t* http1_header_create(const char* key, size_t key_length, const char* value, size_t value_length) {
    http1_header_t* header = http1_header_alloc();

    if (header == NULL) return NULL;

    header->key = http1_set_field(key, key_length);
    header->key_length = key_length;
    header->value = http1_set_field(value, value_length);
    header->value_length = value_length;
    header->next = NULL;

    return header;
}

http1_query_t* http1_query_alloc() {
    return (http1_query_t*)malloc(sizeof(http1_query_t));
}

http1_query_t* http1_query_create(const char* key, size_t key_length, const char* value, size_t value_length) {
    http1_query_t* query = http1_query_alloc();

    if (query == NULL) return NULL;

    query->key = http1_set_field(key, key_length);
    query->value = http1_set_field(value, value_length);
    query->next = NULL;

    return query;
}

const char* http1_set_field(const char* string, size_t length) {
    if (string == NULL) return NULL;

    char* value = (char*)malloc(length + 1);

    if (value == NULL) return value;

    strcpy(value, string);

    return value;
}
