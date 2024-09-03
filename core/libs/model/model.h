#ifndef __MODEL__
#define __MODEL__

#include <stddef.h>
#include <string.h>

#include "json.h"
#include "array.h"
#include "str.h"

#define mparameter_string(NAME, VALUE) \
    {\
        .type = MODEL_STRING,\
        .name = #NAME,\
        .dirty = 0,\
        .value._int = 0,\
        .value._string = str_create_st(VALUE, strlen(VALUE != NULL ? VALUE : "")),\
        .oldvalue._int = 0,\
        .oldvalue._string = str_create_null()\
    }

#define mparameter_int(NAME, VALUE) \
    {\
        .type = MODEL_INT,\
        .name = #NAME,\
        .dirty = 0,\
        .value._int = VALUE,\
        .value._string = str_create_null(),\
        .oldvalue._int = 0,\
        .oldvalue._string = str_create_null()\
    }

#define mparameter_double(NAME, VALUE) \
    {\
        .type = MODEL_DOUBLE,\
        .name = #NAME,\
        .dirty = 0,\
        .value._double = VALUE,\
        .value._string = str_create_null(),\
        .oldvalue._int = 0,\
        .oldvalue._string = str_create_null()\
    }




#define mfield_string(NAME, VALUE) \
    .NAME = {\
        .type = MODEL_STRING,\
        .name = #NAME,\
        .dirty = 0,\
        .value._int = 0,\
        .value._string = str_create_st(VALUE, strlen(VALUE != NULL ? VALUE : "")),\
        .oldvalue._int = 0,\
        .oldvalue._string = str_create_null()\
    }

#define mfield_int(NAME, VALUE) \
    .NAME = {\
        .type = MODEL_INT,\
        .name = #NAME,\
        .dirty = 0,\
        .value._int = VALUE,\
        .value._string = str_create_null(),\
        .oldvalue._int = 0,\
        .oldvalue._string = str_create_null()\
    }

typedef enum {
    MODEL_INT = 0,
    MODEL_DOUBLE,
    MODEL_BOOL,
    MODEL_DATE,
    MODEL_TIME,
    MODEL_DATETIME,
    MODEL_JSON,
    MODEL_STRING,
    MODEL_NULL,
} mtype_e;

typedef struct {
    union {
        int _int;
        long long _llong;
        double _double;
    };
    str_t _string;
} mvalue_t;

typedef struct mfield {
    const mtype_e type;
    unsigned dirty : 1;
    char name[64];
    mvalue_t value;
    mvalue_t oldvalue;
} mfield_t, mfield_int_t, mfield_string_t, mfield_double_t,
  mfield_bool_t, mfield_date_t, mfield_time_t,
  mfield_datetime_t, mfield_json_t, mfield_null_t;

typedef struct model {
    mfield_t*(*first_field)(void* arg);
    int(*fields_count)(void* arg);
    const char*(*table)(void* arg);
    const char**(*primary_key)(void* arg);
    int(*primary_key_count)(void* arg);
} model_t;

typedef struct modelview {
    mfield_t*(*first_field)(void* arg);
    int(*fields_count)(void* arg);
} modelview_t;

int model_get(const char* dbid, void* arg, mfield_t* params, int params_count);
int model_create(const char* dbid, void* arg);
int model_update(const char* dbid, void* arg);
int model_delete(const char* dbid, void* arg);
int model_execute(const char* dbid, const char* format, ...);
jsontok_t* model_to_json(void* arg, jsondoc_t* document);
char* model_stringify(void* arg);
char* model_list_stringify(array_t* array);
void model_free(void* arg);

void* model_one(const char* dbid, void*(create_instance)(void), const char* format, ...);
array_t* model_list(const char* dbid, void*(create_instance)(void), const char* format, ...);

int model_int(mfield_int_t* field);
const char* model_string(mfield_string_t* field);
double model_double(mfield_double_t* field);

void model_set_int(mfield_int_t* field, int value);
void model_set_string(mfield_string_t* field, const char* value);
void model_set_stringn(mfield_string_t* field, const char* value, size_t size);
void model_set_double(mfield_double_t* field, double value);

int mparam_int(mfield_int_t* param);
double mparam_double(mfield_double_t* param);
const char* mparam_string(mfield_string_t* param);

const char* mvalue_to_string(mvalue_t* field, const mtype_e type);
int mstr_to_int(const char* value);

void mvalue_clear(mvalue_t* value);

#endif