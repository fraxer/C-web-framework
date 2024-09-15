#include <string.h>

#include "str.h"

int __str_expand_buffer(str_t* str, const size_t extra_size);

str_t* str_create(const char* string, const size_t size) {
    str_t* data = str_create_empty();
    if (data == NULL) return NULL;

    if (!str_assign(data, string, size)) {
        str_free(data);
        return NULL;
    }

    return data;
}

str_t* str_create_empty(void) {
    str_t* data = malloc(sizeof * data);
    if (data == NULL) return NULL;

    str_init(data);

    return data;
}

int str_init(str_t* str) {
    if (str == NULL) return 0;

    str->buffer = NULL;
    str->size = 0;
    str->capacity = 0;

    return 1;
}

int str_reset(str_t* str) {
    char* data = realloc(str->buffer, STR_BUFFER_SIZE);
    if (data == NULL) return 0;

    str->size = 0;
    str->capacity = STR_BUFFER_SIZE;
    str->buffer = data;

    return 1;
}

void str_clear(str_t* str) {
    if (str == NULL) return;
    if (str->buffer != NULL)
        free(str->buffer);

    str_init(str);
}

void str_free(str_t* str) {
    if (str == NULL) return;

    str_clear(str);
    free(str);
}

size_t str_size(str_t* str) {
    return str->size;
}

int str_insertc(str_t* str, char ch, size_t pos) {
    if (str == NULL || str->buffer == NULL)
        return 0;

    if (str->buffer == NULL)
        if (!str_reset(str))
            return 0;

    if (pos > str->size)
        return 0;

    if (str->size + 1 >= str->capacity)
        if (!__str_expand_buffer(str, 0))
            return 0;

    memmove(str->buffer + pos + 1, str->buffer + pos, str->size - pos + 1);
    str->buffer[pos] = ch;
    str->size++;
    str->buffer[str->size] = 0;

    return 1;
}

int str_prependc(str_t* str, char ch) {
    return str_insertc(str, ch, 0);
}

int str_appendc(str_t* str, char ch) {
    return str_insertc(str, ch, str->size);
}

int str_insert(str_t* str, const char* string, size_t size, size_t pos) {
    if (str == NULL || str->buffer == NULL || string == NULL)
        return 0;

    if (str->buffer == NULL)
        if (!str_reset(str))
            return 0;

    if (pos > str->size)
        return 0;

    if (str->size + size + 1 >= str->capacity)
        if (!__str_expand_buffer(str, size))
            return 0;

    memmove(str->buffer + pos + size, str->buffer + pos, str->size - pos);
    memcpy(str->buffer + pos, string, size);
    str->size += size;
    str->buffer[str->size] = 0;

    return 1;
}

int str_prepend(str_t* str, const char* string, size_t size) {
    return str_insert(str, string, size, 0);
}

int str_append(str_t* str, const char* string, size_t size) {
    return str_insert(str, string, size, str->size);
}

int str_assign(str_t* str, const char* string, size_t size) {
    str->size = 0;

    return str_insert(str, string, size, 0);
}

int str_move(str_t* srcstr, str_t* dststr) {
    if (srcstr == NULL || dststr == NULL)
        return 0;

    if (dststr->buffer != NULL)
        str_clear(dststr);

    dststr->buffer = srcstr->buffer;
    dststr->size = srcstr->size;
    dststr->capacity = srcstr->capacity;

    return str_init(srcstr);
}

char* str_get(str_t* str) {
    return str->buffer;
}

char* str_copy(str_t* str) {
    char* string = str_get(str);
    size_t length = str_size(str);

    char* data = malloc(length + 1);
    if (data == NULL) return NULL;

    memcpy(data, string, length);
    data[length] = 0;

    return data;
}

int __str_expand_buffer(str_t* str, const size_t extra_size) {
    size_t target_size = str->capacity + STR_BUFFER_SIZE;
    if (extra_size > 0)
        target_size += extra_size;

    void* data = realloc(str->buffer, target_size);
    if (data == NULL) return 0;

    str->buffer = data;
    str->capacity = target_size;

    return 1;
}
