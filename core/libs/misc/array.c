#include <string.h>

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

void array_free(array_t* array) {
    for (size_t i = 0; i < array->size; i++)
        __array_free_value(&array->elements[i]);

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

avalue_t array_create_string(const char* value, size_t size) {
    avalue_t v = {
        .type = ARRAY_STRING,
        ._int = 0,
        ._length = size,
        ._string = strdup(value)
    };

    if (v._string == NULL) {
        v._length = 0;
    }

    return v;
}

avalue_t array_create_pointer(void* pointer, void(*free_pointer)(void*)) {
    avalue_t v = {
        .type = ARRAY_POINTER,
        ._int = 0,
        ._length = 0,
        ._string = 0,
        ._pointer = pointer,
        ._free_pointer = free_pointer
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
    switch (value->type) {
        case ARRAY_STRING:
            free(value->_string);
            value->_string = NULL;
            value->_length = 0;
            break;
        case ARRAY_POINTER:
            value->_free_pointer(value->_pointer);
            value->_free_pointer = NULL;
            value->_pointer = NULL;
            break;
        default:
            break;
    }
}
