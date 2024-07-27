#include <stdlib.h>
#include <string.h>

#include "mailheader.h"

char* __mail_set_field(const char* string, const size_t length);

mail_header_t* mail_header_create(const char* key, const size_t key_length, const char* value, const size_t value_length) {
    mail_header_t* header = malloc(sizeof * header);
    if (header == NULL) return NULL;

    header->key = NULL;
    header->key_length = key_length;
    header->value = NULL;
    header->value_length = value_length;
    header->next = NULL;

    if (key_length)
        header->key = __mail_set_field(key, key_length);

    if (value_length)
        header->value = __mail_set_field(value, value_length);

    return header;
}

void mail_header_free(mail_header_t* header) {
    if (header->key)
        free(header->key);

    if (header->value)
        free(header->value);

    free(header);
}

char* __mail_set_field(const char* string, const size_t length) {
    char* value = malloc(length + 1);
    if (value == NULL) return NULL;

    if (string != NULL) {
        memcpy(value, string, length);
        value[length] = 0;
    }

    return value;
}
