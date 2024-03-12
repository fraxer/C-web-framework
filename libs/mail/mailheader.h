#ifndef __MAILHEADER__
#define __MAILHEADER__

#include <stddef.h>

typedef struct mail_header {
    char* key;
    char* value;
    size_t key_length;
    size_t value_length;
    struct mail_header* next;
} mail_header_t;

mail_header_t* mail_header_create(const char* key, const size_t key_length, const char* value, const size_t value_length);
void mail_header_free(mail_header_t* header);

#endif