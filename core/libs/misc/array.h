#ifndef __ARRAY__
#define __ARRAY__

#include <stdlib.h>

typedef enum {
    ARRAY_INT,
    ARRAY_DOUBLE,
    ARRAY_STRING,
    ARRAY_POINTER
} atype_t;

typedef struct {
    atype_t type;
    union {
        int _int;
        long long _long;
        double _double;
    };
    struct {
        size_t _length;
        char* _string;
    };
    struct {
        void* _pointer;
        void(*_free_pointer)(void*);
    };
} avalue_t;

typedef struct {
    avalue_t* elements;
    size_t size;
    size_t capacity;
} array_t;

array_t* array_create(void);
void array_free(array_t* array);
void array_insert(array_t* array, size_t index, avalue_t value);
void array_push_front(array_t* array, avalue_t value);
void array_push_back(array_t* array, avalue_t value);
void array_update(array_t* array, size_t index, avalue_t value);
void array_delete(array_t* array, size_t index);
size_t array_size(array_t* array);

avalue_t array_create_int(int value);
avalue_t array_create_double(double value);
avalue_t array_create_string(const char* value, size_t size);
avalue_t array_create_pointer(void* pointer, void(*free_pointer)(void*));

#endif