#ifndef __MODEL__
#define __MODEL__

#include <stddef.h>
#include <string.h>
#include <time.h>

#include "json.h"
#include "array.h"
#include "str.h"

#define mparameter_string(NAME, VALUE) \
    {\
        .type = MODEL_TEXT,\
        .name = #NAME,\
        .dirty = 0,\
        .value._int = 0,\
        .value._string = str_create_str(VALUE, strlen(VALUE != NULL ? VALUE : "")),\
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
        .type = MODEL_TEXT,\
        .name = #NAME,\
        .dirty = 0,\
        .value._int = 0,\
        .value._string = str_create_str(VALUE, strlen(VALUE != NULL ? VALUE : "")),\
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
    MODEL_BOOL = 0,

    MODEL_SMALLINT,
    MODEL_INT,
    MODEL_BIGINT,
    MODEL_FLOAT,
    MODEL_DOUBLE,
    MODEL_DECIMAL,
    MODEL_MONEY,

    MODEL_DATE,
    MODEL_TIME,
    MODEL_TIMETZ,
    MODEL_TIMESTAMP,
    MODEL_TIMESTAMPTZ,

    MODEL_JSON,

    MODEL_BINARY,
    MODEL_VARCHAR,
    MODEL_CHAR,
    MODEL_TEXT,
    MODEL_ENUM,
    MODEL_NULL,
} mtype_e;

typedef struct {
    union {
        unsigned int _null : 1;
        short _short;
        int _int;
        long long _bigint;
        float _float;
        double _double;
        long double _ldouble;
        struct tm _tm;
        jsondoc_t* _jsondoc;
    };
    str_t _string;
    size_t _string_max_size;
} mvalue_t;

typedef struct mfield {
    const mtype_e type;
    unsigned dirty : 1;
    char name[64];
    mvalue_t value;
    mvalue_t oldvalue;
} mfield_t, mfield_int_t, mfield_string_t, mfield_double_t,
  mfield_bool_t, mfield_date_t, mfield_time_t, mfield_json_t, mfield_null_t;

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
void* model_one(const char* dbid, void*(create_instance)(void), const char* format, ...);
array_t* model_list(const char* dbid, void*(create_instance)(void), const char* format, ...);
int model_execute(const char* dbid, const char* format, ...);

jsontok_t* model_to_json(void* arg, jsondoc_t* document);
char* model_stringify(void* arg);
char* model_list_stringify(array_t* array);
void model_free(void* arg);




short model_bool(mfield_int_t* field);
short model_smallint(mfield_int_t* field);
int model_int(mfield_int_t* field);
long long int model_bigint(mfield_int_t* field);
float model_float(mfield_int_t* field);
double model_double(mfield_double_t* field);
long double model_decimal(mfield_int_t* field);
double model_money(mfield_int_t* field);

struct tm* model_timestamp(mfield_int_t* field);
struct tm* model_timestamptz(mfield_int_t* field);
struct tm* model_date(mfield_int_t* field);
struct tm* model_time(mfield_int_t* field);
struct tm* model_timetz(mfield_int_t* field);

jsondoc_t* model_json(mfield_int_t* field);

str_t* model_binary(mfield_int_t* field);
str_t* model_varchar(mfield_int_t* field);
str_t* model_char(mfield_int_t* field);
str_t* model_text(mfield_int_t* field);
str_t* model_enum(mfield_int_t* field);

const char* model_string(mfield_string_t* field);




int model_set_bool(mfield_t* field, short value);
int model_set_smallint(mfield_t* field, short value);
int model_set_int(mfield_t* field, int value);
int model_set_bigint(mfield_t* field, long long int value);
int model_set_float(mfield_t* field, float value);
int model_set_double(mfield_t* field, double value);
int model_set_decimal(mfield_t* field, long double value);
int model_set_money(mfield_t* field, double value);

int model_set_timestamp(mfield_t* field, struct tm* value);
int model_set_timestamptz(mfield_t* field, struct tm* value);
int model_set_date(mfield_t* field, struct tm* value);
int model_set_time(mfield_t* field, struct tm* value);
int model_set_timetz(mfield_t* field, struct tm* value);

int model_set_json(mfield_t* field, jsondoc_t* value);

int model_set_binary(mfield_t* field, str_t* value);
int model_set_varchar(mfield_t* field, str_t* value);
int model_set_char(mfield_t* field, str_t* value);
int model_set_text(mfield_t* field, str_t* value);
int model_set_enum(mfield_t* field, str_t* value);




int model_set_bool_from_str(mfield_t* field, const char* value);
int model_set_smallint_from_str(mfield_t* field, const char* value);
int model_set_int_from_str(mfield_t* field, const char* value);
int model_set_bigint_from_str(mfield_t* field, const char* value);
int model_set_float_from_str(mfield_t* field, const char* value);
int model_set_double_from_str(mfield_t* field, const char* value);
int model_set_decimal_from_str(mfield_t* field, const char* value);
int model_set_money_from_str(mfield_t* field, const char* value);

int model_set_timestamp_from_str(mfield_t* field, const char* value);
int model_set_timestamptz_from_str(mfield_t* field, const char* value);
int model_set_date_from_str(mfield_t* field, const char* value);
int model_set_time_from_str(mfield_t* field, const char* value);
int model_set_timetz_from_str(mfield_t* field, const char* value);

int model_set_json_from_str(mfield_t* field, const char* value);

int model_set_binary_from_str(mfield_t* field, const char* value, size_t size);
int model_set_varchar_from_str(mfield_t* field, const char* value, size_t size);
int model_set_char_from_str(mfield_t* field, const char* value, size_t size);
int model_set_text_from_str(mfield_t* field, const char* value, size_t size);
int model_set_enum_from_str(mfield_t* field, const char* value, size_t size);




// short int model_str_to_bool(mfield_t* field);
// short int model_str_to_smallint(mfield_t* field);
// int model_str_to_int(const char* value);
// long long int model_str_to_bigint(mfield_t* field);
// float model_str_to_float(mfield_t* field);
// double model_str_to_double(mfield_t* field);
// long double model_str_to_decimal(mfield_t* field);
// long double model_str_to_money(mfield_t* field);

// struct tm* model_str_to_timestamp(mfield_t* field);
// struct tm* model_str_to_timestamptz(mfield_t* field);
// struct tm* model_str_to_date(mfield_t* field);
// struct tm* model_str_to_time(mfield_t* field);
// struct tm* model_str_to_timetz(mfield_t* field);

// jsondoc_t* model_str_to_json(mfield_t* field);

// str_t* model_str_to_binary(mfield_t* field);
// str_t* model_str_to_varchar(mfield_t* field);
// str_t* model_str_to_char(mfield_t* field);
// str_t* model_str_to_text(mfield_t* field);
// str_t* model_str_to_enum(mfield_t* field);



const char* mvalue_to_string(mvalue_t* field, const mtype_e type);
void mvalue_clear(mvalue_t* value);

#endif