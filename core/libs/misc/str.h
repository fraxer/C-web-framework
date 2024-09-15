#ifndef __STR__
#define __STR__

#include <stdlib.h>

#define STR_BUFFER_SIZE 4096

typedef struct {
    char* buffer;
    size_t size;
    size_t capacity;
} str_t;

str_t* str_create(const char* string, const size_t size);
str_t* str_create_empty(void);
int str_init(str_t* str);
int str_reset(str_t* str);
void str_clear(str_t* str);
void str_free(str_t* str);

size_t str_size(str_t* str);
int str_insertc(str_t* str, char ch, size_t pos);
int str_prependc(str_t* str, char ch);
int str_appendc(str_t* str, char ch);
int str_insert(str_t* str, const char* string, size_t size, size_t pos);
int str_prepend(str_t* str, const char* string, size_t size);
int str_append(str_t* str, const char* string, size_t size);
int str_assign(str_t* str, const char* string, size_t size);
int str_move(str_t* srcstr, str_t* dststr);

char* str_get(str_t* str);
char* str_copy(str_t* str);

#endif