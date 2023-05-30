#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "http1common.h"

http1_header_t* http1_header_alloc();
http1_query_t* http1_query_alloc();
char http1_from_hex(char);
char http1_to_hex(char);


http1_header_t* http1_header_alloc() {
    return malloc(sizeof(http1_header_t));
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

void http1_header_free(http1_header_t* header) {
    free((void*)header->key);
    free((void*)header->value);
    free(header);
}

http1_query_t* http1_query_alloc() {
    return malloc(sizeof(http1_query_t));
}

http1_query_t* http1_query_create(const char* key, size_t key_length, const char* value, size_t value_length) {
    http1_query_t* query = http1_query_alloc();

    if (query == NULL) return NULL;

    query->key = http1_set_field(key, key_length);
    query->value = http1_set_field(value, value_length);
    query->next = NULL;

    return query;
}

void http1_query_free(http1_query_t* query) {
    free((void*)query->key);
    free((void*)query->value);
    free(query);
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

char http1_from_hex(char ch) {
  return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
}

char http1_to_hex(char code) {
  static char hex[] = "0123456789abcdef";
  return hex[code & 15];
}

http1_urlendec_t http1_urlencode(const char* string, size_t length) {
    char* buffer = malloc(length * 3 + 1);
    char* pbuffer = buffer;

    while (*string) {
        if (isalnum(*string) || *string == '-' || *string == '_' || *string == '.' || *string == '~') {
            *pbuffer++ = *string;
        }
        else if (*string == ' ') {
            *pbuffer++ = '+';
        }
        else {
            *pbuffer++ = '%';
            *pbuffer++ = http1_to_hex(*string >> 4);
            *pbuffer++ = http1_to_hex(*string & 15);
        }
        string++;
    }

    *pbuffer = 0;

    return (http1_urlendec_t){
        .string = buffer,
        .length = pbuffer - buffer
    };
}

http1_urlendec_t http1_urldecode(const char* string, size_t length) {
    char* buffer = malloc(length + 1);
    char* pbuffer = buffer;

    while (*string) {
        if (*string == '%') {
            if (string[1] && string[2]) {
                *pbuffer++ = http1_from_hex(string[1]) << 4 | http1_from_hex(string[2]);
                string += 2;
            }
        } else if (*string == '+') { 
            *pbuffer++ = ' ';
        } else {
            *pbuffer++ = *string;
        }
        string++;
    }

    *pbuffer = 0;

    return (http1_urlendec_t){
        .string = buffer,
        .length = pbuffer - buffer
    };
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
