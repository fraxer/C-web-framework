#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "http1common.h"

http1_header_t* http1_header_alloc();
http1_query_t* http1_query_alloc();


http1_header_t* http1_header_alloc() {
    return (http1_header_t*)malloc(sizeof(http1_header_t));
}

http1_header_t* http1_header_create(const char* key, size_t key_langth, const char* value, size_t value_langth) {
    http1_header_t* header = http1_header_alloc();

    if (header == NULL) return NULL;

    header->key = key;
    header->key_length = key_langth;
    header->value = value;
    header->value_length = value_langth;
    header->next = NULL;

    return header;
}

http1_query_t* http1_query_alloc() {
    return (http1_query_t*)malloc(sizeof(http1_query_t));
}

http1_query_t* http1_query_create() {
    http1_query_t* query = http1_query_alloc();

    if (query == NULL) return NULL;

    query->key = NULL;
    query->value = NULL;
    query->next = NULL;

    return query;
}

const char* http1_query_set_field(const char* string, size_t length) {
    char* field = (char*)calloc(length + 1, sizeof(char));

    if (field == NULL) return NULL;

    strncpy(field, string, length);

    field[length] = 0;

    return field;
}