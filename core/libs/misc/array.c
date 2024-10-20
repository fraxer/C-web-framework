#include <string.h>
#include <stdio.h>

#include "log.h"
#include "array.h"

#define INITIAL_CAPACITY 10

void __array_resize(array_t* array, size_t new_capacity);
void __array_free_value(avalue_t* value);

array_t* array_create(void) {
    array_t* array = malloc(sizeof * array);
    if (array == NULL)
        return NULL;

    array->elements = malloc(INITIAL_CAPACITY * sizeof(avalue_t));
    if (array->elements == NULL) {
        free(array);
        return NULL;
    }

    array->size = 0;
    array->capacity = INITIAL_CAPACITY;

    return array;
}

array_t* array_create_from_ints(int* ints, size_t count) {
    array_t* array = array_create();
    if (array == NULL) return NULL;

    for (size_t i = 0; i < count; i++)
        array_push_back_int(array, ints[i]);

    return array;
}

array_t *array_create_from_doubles(double* doubles, size_t count) {
    array_t* array = array_create();
    if (array == NULL) return NULL;

    for (size_t i = 0; i < count; i++)
        array_push_back_double(array, doubles[i]);

    return array;
}

array_t* array_create_from_strings(char** strings, size_t count) {
    array_t* array = array_create();
    if (array == NULL) return NULL;

    for (size_t i = 0; i < count; i++)
        array_push_back_str(array, strings[i]);

    return array;
}

array_t* array_copy(array_t* array) {
    if (array == NULL) return NULL;

    array_t* copy_array = array_create();
    if (copy_array == NULL) return NULL;

    for (size_t i = 0; i < array_size(array); i++) {
        avalue_t* element = &array->elements[i];

        if (element->type == ARRAY_INT)
            array_push_back(copy_array, array_create_int(element->_int));
        if (element->type == ARRAY_DOUBLE)
            array_push_back(copy_array, array_create_double(element->_double));
        if (element->type == ARRAY_STRING)
            array_push_back(copy_array, array_create_stringn(element->_string, element->_length));
        if (element->type == ARRAY_POINTER)
            array_push_back(copy_array, array_create_pointer(element->_copy(element->_pointer), element->_copy, element->_free));
    }

    return copy_array;
}

void array_clear(array_t* array) {
    if (array == NULL) return;

    for (size_t i = 0; i < array->size; i++)
        __array_free_value(&array->elements[i]);

    array->size = 0;
}

void array_free(array_t* array) {
    if (array == NULL) return;

    array_clear(array);
    free(array->elements);
    free(array);
}

void array_insert(array_t* array, size_t index, avalue_t value) {
    if (array == NULL) return;
    if (array->elements == NULL) return;
    if (index > array->size) {
        log_error("array_insert: Index out of bounds\n");
        return;
    }
    if (index > array->capacity) {
        log_error("array_insert: Index out of bounds\n");
        return;
    }

    if (array->size >= array->capacity)
        __array_resize(array, array->capacity + INITIAL_CAPACITY);

    if (index < array->size)
        memmove(&array->elements[index + 1], &array->elements[index], (array->size - index) * sizeof(avalue_t));

    array->elements[index] = value;
    array->size++;
}

void array_push_front(array_t* array, avalue_t value) {
    array_insert(array, 0, value);
}

void array_push_back(array_t* array, avalue_t value) {
    array_insert(array, array->size, value);
}

void array_push_back_str(array_t* array, const char* value) {
    array_push_back(array, array_create_string(value));
}

void array_push_back_int(array_t* array, int value) {
    array_push_back(array, array_create_int(value));
}

void array_push_back_double(array_t* array, double value) {
    array_push_back(array, array_create_double(value));
}

void array_update(array_t* array, size_t index, avalue_t value) {
    if (index >= array->size) {
        log_error("array_update: Index out of bounds\n");
        return;
    }

    __array_free_value(&array->elements[index]);

    array->elements[index] = value;
}

void array_delete(array_t* array, size_t index) {
    if (index >= array->size) {
        log_error("array_delete: Index out of bounds\n");
        return;
    }

    __array_free_value(&array->elements[index]);

    if (index < array->size - 1)
        memmove(&array->elements[index], &array->elements[index + 1], (array->size - index - 1) * sizeof(avalue_t));

    array->size--;

    if (array->size <= array->capacity - INITIAL_CAPACITY && array->capacity > INITIAL_CAPACITY)
        __array_resize(array, array->capacity - INITIAL_CAPACITY);
}

size_t array_size(array_t* array) {
    if (array == NULL) return 0;

    return array->size;
}

void* array_get(array_t* array, size_t index) {
    if (array == NULL) return NULL;
    if (index >= array->size) {
        log_error("array_get: Index out of bounds\n");
        return NULL;
    }

    avalue_t* element = &array->elements[index];

    if (element->type == ARRAY_INT)
        return &element->_int;
    if (element->type == ARRAY_DOUBLE)
        return &element->_double;
    if (element->type == ARRAY_STRING)
        return element->_string;

    return element->_pointer;
}

int array_get_int(array_t* array, size_t index) {
    void* value = array_get(array, index);
    if (value == NULL) return 0;

    return *(int*)value;
}

double array_get_double(array_t* array, size_t index) {
    void* value = array_get(array, index);
    if (value == NULL) return 0.0;

    return *(double*)value;
}

const char* array_get_string(array_t* array, size_t index) {
    return array_get(array, index);
}

void* array_get_pointer(array_t* array, size_t index) {
    return array_get(array, index);
}

str_t* array_item_to_string(array_t* array, size_t index) {
    if (array == NULL) return NULL;
    if (index >= array->size) {
        log_error("array_get: Index out of bounds\n");
        return NULL;
    }

    avalue_t* element = &array->elements[index];

    str_t* string = str_create_empty();
    if (string == NULL) return NULL;

    if (element->type == ARRAY_STRING) {
        const char* str = array_get_string(array, index);
        if (str == NULL) {
            str_free(string);
            return NULL;
        }

        str_append(string, str, strlen(str));
    }
    else if (element->type == ARRAY_DOUBLE) {
        char str[375] = {0};
        int size = snprintf(str, sizeof(str), "%.12f", array_get_double(array, index));
        if (size < 0) {
            str_free(string);
            return NULL;
        }

        str_append(string, str, size);
    }
    else if (element->type == ARRAY_INT) {
        char str[12] = {0};
        int size = snprintf(str, sizeof(str), "%d", array_get_int(array, index));
        if (size < 0) {
            str_free(string);
            return NULL;
        }

        str_append(string, str, size);
    }
    else {
        str_free(string);
        return NULL;
    }

    return string;
}

avalue_t array_create_int(int value) {
    avalue_t v = {
        .type = ARRAY_INT,
        ._int = value,
        ._length = 0,
        ._string = NULL
    };

    return v;
}

avalue_t array_create_double(double value) {
    avalue_t v = {
        .type = ARRAY_DOUBLE,
        ._double = value,
        ._length = 0,
        ._string = NULL
    };

    return v;
}

avalue_t array_create_string(const char* value) {
    return array_create_stringn(value, strlen(value));
}

avalue_t array_create_stringn(const char* value, size_t size) {
    avalue_t v = {
        .type = ARRAY_STRING,
        ._int = 0,
        ._length = 0,
        ._string = NULL,
        ._pointer = NULL,
        ._copy = NULL,
        ._free = NULL
    };

    char* string = malloc(size + 1);
    if (string == NULL) return v;

    strncpy(string, value, size);
    string[size] = 0;

    if (string != NULL) {
        v._string = string;
        v._length = size;
    }

    return v;
}

avalue_t array_create_pointer(void* pointer, void*(*oncopy)(void*), void(*onfree)(void*))
{
    avalue_t v = {
        .type = ARRAY_POINTER,
        ._int = 0,
        ._length = 0,
        ._string = 0,
        ._pointer = pointer,
        ._copy = oncopy,
        ._free = onfree,
    };

    return v;
}

void __array_resize(array_t* array, size_t new_capacity) {
    void* data = realloc(array->elements, new_capacity * sizeof(avalue_t));
    if (data == NULL) return;

    array->elements = data;
    array->capacity = new_capacity;
}

void __array_free_value(avalue_t* value) {
    if (value->type == ARRAY_STRING)
        if (value->_string != NULL)
            free(value->_string);

    if (value->type == ARRAY_POINTER)
        if (value->_free != NULL)
            value->_free(value->_pointer);

    value->type = ARRAY_INT;
    value->_int = 0;
    value->_length = 0;
    value->_string = NULL;
    value->_copy = NULL;
    value->_free = NULL;
    value->_pointer = NULL;
}
