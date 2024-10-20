#ifndef __ARRAY__
#define __ARRAY__

#include <stdlib.h>
#include "str.h"
#include "macros.h"

#define array_nocopy NULL
#define array_nofree NULL
#define array_create_ints(...) array_create_from_ints(args_int(__VA_ARGS__))
#define array_create_doubles(...) array_create_from_doubles(args_double(__VA_ARGS__))
#define array_create_strings(...) array_create_from_strings(args_str(__VA_ARGS__))

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
        void*(*_copy)(void*);
        void(*_free)(void*);
    };
} avalue_t;

typedef struct {
    avalue_t* elements;
    size_t size;
    size_t capacity;
} array_t;

array_t* array_create(void);
array_t* array_create_from_ints(int* ints, size_t count);
array_t* array_create_from_doubles(double* doubles, size_t count);
array_t* array_create_from_strings(char** strings, size_t count);
array_t* array_copy(array_t* array);
void array_clear(array_t* array);
void array_free(array_t* array);
void array_insert(array_t* array, size_t index, avalue_t value);
void array_push_front(array_t* array, avalue_t value);
void array_push_back(array_t* array, avalue_t value);
void array_push_back_str(array_t* array, const char* value);
void array_push_back_int(array_t* array, int value);
void array_push_back_double(array_t* array, double value);
void array_update(array_t* array, size_t index, avalue_t value);
void array_delete(array_t* array, size_t index);
size_t array_size(array_t* array);
void* array_get(array_t* array, size_t index);
int array_get_int(array_t* array, size_t index);
double array_get_double(array_t* array, size_t index);
const char* array_get_string(array_t* array, size_t index);
void* array_get_pointer(array_t* array, size_t index);
str_t* array_item_to_string(array_t* array, size_t index);

avalue_t array_create_int(int value);
avalue_t array_create_double(double value);
avalue_t array_create_string(const char* value);
avalue_t array_create_stringn(const char* value, size_t size);
avalue_t array_create_pointer(void* pointer, void*(*oncopy)(void*), void(*onfree)(void*));

#endif