#ifndef __MODEL__
#define __MODEL__

#include <stddef.h>
#include <string.h>
#include <time.h>

#include "json.h"
#include "array.h"
#include "str.h"
#include "enums.h"
#include "macros.h"

typedef struct tm tm_t;

tm_t* tm_create(tm_t* time);

#define mnfields(TYPE, NAME, VALUE) \
    .type = TYPE,\
    .name = #NAME,\
    .dirty = 0,\
    .value._short = VALUE,\
    .value._string = NULL,\
    .oldvalue._short = 0,\
    .oldvalue._string = NULL

#define mdfields(TYPE, NAME, VALUE) \
    .type = TYPE,\
    .name = #NAME,\
    .dirty = 0,\
    .value._tm = tm_create(VALUE),\
    .value._string = NULL,\
    .oldvalue._tm = NULL,\
    .oldvalue._string = NULL

#define msfields(TYPE, NAME, VALUE) \
    .type = TYPE,\
    .name = #NAME,\
    .dirty = 0,\
    .value._short = 0,\
    .value._string = str_create(VALUE, strlen(VALUE != NULL ? VALUE : "")),\
    .oldvalue._short = 0,\
    .oldvalue._string = NULL

#define mparam_bool(NAME, VALUE) { mnfields(MODEL_BOOL, NAME, VALUE) }
#define mparam_smallint(NAME, VALUE) { mnfields(MODEL_SMALLINT, NAME, VALUE) }
#define mparam_int(NAME, VALUE) { mnfields(MODEL_INT, NAME, VALUE) }
#define mparam_bigint(NAME, VALUE) { mnfields(MODEL_BIGINT, NAME, VALUE) }
#define mparam_float(NAME, VALUE) { mnfields(MODEL_FLOAT, NAME, VALUE) }
#define mparam_double(NAME, VALUE) { mnfields(MODEL_DOUBLE, NAME, VALUE) }
#define mparam_decimal(NAME, VALUE) { mnfields(MODEL_DECIMAL, NAME, VALUE) }
#define mparam_money(NAME, VALUE) { mnfields(MODEL_MONEY, NAME, VALUE) }
#define mparam_date(NAME, VALUE) { mdfields(MODEL_DATE, NAME, VALUE) }
#define mparam_time(NAME, VALUE) { mdfields(MODEL_TIME, NAME, VALUE) }
#define mparam_timetz(NAME, VALUE) { mdfields(MODEL_TIMETZ, NAME, VALUE) }
#define mparam_timestamp(NAME, VALUE) { mdfields(MODEL_TIMESTAMP, NAME, VALUE) }
#define mparam_timestamptz(NAME, VALUE) { mdfields(MODEL_TIMESTAMPTZ, NAME, VALUE) }
#define mparam_json(NAME, VALUE) \
    {\
        .type = MODEL_JSON,\
        .name = #NAME,\
        .dirty = 0,\
        .value._jsondoc = json_create(VALUE),\
        .value._string = NULL,\
        .oldvalue._jsondoc = NULL,\
        .oldvalue._string = NULL\
    }

#define mparam_binary(NAME, VALUE) { msfields(MODEL_BINARY, NAME, VALUE) }
#define mparam_varchar(NAME, VALUE) { msfields(MODEL_VARCHAR, NAME, VALUE) }
#define mparam_char(NAME, VALUE) { msfields(MODEL_CHAR, NAME, VALUE) }
#define mparam_text(NAME, VALUE) { msfields(MODEL_TEXT, NAME, VALUE) }
#define mparam_enum(NAME, VALUE, ...) \
    {\
        .type = MODEL_ENUM,\
        .name = #NAME,\
        .dirty = 0,\
        .value._enum = enums_create(args_str(__VA_ARGS__)),\
        .value._string = str_create(VALUE, strlen(VALUE != NULL ? VALUE : "")),\
        .oldvalue._enum = NULL,\
        .oldvalue._string = NULL\
    }

#define mfield_bool(NAME, VALUE) .NAME = mparam_bool(NAME, VALUE)
#define mfield_smallint(NAME, VALUE) .NAME = mparam_smallint(NAME, VALUE)
#define mfield_int(NAME, VALUE) .NAME = mparam_int(NAME, VALUE)
#define mfield_bigint(NAME, VALUE) .NAME = mparam_bigint(NAME, VALUE)
#define mfield_float(NAME, VALUE) .NAME = mparam_float(NAME, VALUE)
#define mfield_double(NAME, VALUE) .NAME = mparam_double(NAME, VALUE)
#define mfield_decimal(NAME, VALUE) .NAME = mparam_decimal(NAME, VALUE)
#define mfield_money(NAME, VALUE) .NAME = mparam_money(NAME, VALUE)
#define mfield_date(NAME, VALUE) .NAME = mparam_date(NAME, VALUE)
#define mfield_time(NAME, VALUE) .NAME = mparam_time(NAME, VALUE)
#define mfield_timetz(NAME, VALUE) .NAME = mparam_timetz(NAME, VALUE)
#define mfield_timestamp(NAME, VALUE) .NAME = mparam_timestamp(NAME, VALUE)
#define mfield_timestamptz(NAME, VALUE) .NAME = mparam_timestamptz(NAME, VALUE)
#define mfield_json(NAME, VALUE) .NAME = mparam_json(NAME, VALUE)
#define mfield_binary(NAME, VALUE) .NAME = mparam_binary(NAME, VALUE)
#define mfield_varchar(NAME, VALUE) .NAME = mparam_varchar(NAME, VALUE)
#define mfield_char(NAME, VALUE) .NAME = mparam_char(NAME, VALUE)
#define mfield_text(NAME, VALUE) .NAME = mparam_text(NAME, VALUE)
#define mfield_enum(NAME, VALUE, ...) .NAME = mparam_enum(NAME, VALUE, __VA_ARGS__)

#define mfield_props_count 7
#define mparams(...) (mfield_t*)&(mfield_t[NARG_(__VA_ARGS__,MW_NSEQ()) / mfield_props_count]){__VA_ARGS__}, NARG_(__VA_ARGS__,MW_NSEQ()) / mfield_props_count
#define mparams_create(VAR, ...) mfield_t VAR[NARG_(__VA_ARGS__,MW_NSEQ()) / mfield_props_count] = {__VA_ARGS__}
#define mparams_pass(ST) ST[0], sizeof(*ST) / sizeof(mfield_t)
#define mparams_clear(ST) model_params_clear(ST, sizeof(*ST))
#define mparams_free(ST) model_params_free(ST, sizeof(*ST))

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
} mtype_e;

typedef struct {
    union {
        short _short;
        int _int;
        long long _bigint;
        float _float;
        double _double;
        long double _ldouble;
        tm_t* _tm;
        jsondoc_t* _jsondoc;
        enums_t* _enum;
    };
    str_t* _string;
} mvalue_t;

typedef struct mfield {
    const mtype_e type;
    unsigned dirty : 1;
    char name[64];
    mvalue_t value;
    mvalue_t oldvalue;
} mfield_t;

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




short model_bool(mfield_t* field);
short model_smallint(mfield_t* field);
int model_int(mfield_t* field);
long long int model_bigint(mfield_t* field);
float model_float(mfield_t* field);
double model_double(mfield_t* field);
long double model_decimal(mfield_t* field);
double model_money(mfield_t* field);

tm_t* model_timestamp(mfield_t* field);
tm_t* model_timestamptz(mfield_t* field);
tm_t* model_date(mfield_t* field);
tm_t* model_time(mfield_t* field);
tm_t* model_timetz(mfield_t* field);

jsondoc_t* model_json(mfield_t* field);

str_t* model_binary(mfield_t* field);
str_t* model_varchar(mfield_t* field);
str_t* model_char(mfield_t* field);
str_t* model_text(mfield_t* field);
str_t* model_enum(mfield_t* field);





int model_set_bool(mfield_t* field, short value);
int model_set_smallint(mfield_t* field, short value);
int model_set_int(mfield_t* field, int value);
int model_set_bigint(mfield_t* field, long long int value);
int model_set_float(mfield_t* field, float value);
int model_set_double(mfield_t* field, double value);
int model_set_decimal(mfield_t* field, long double value);
int model_set_money(mfield_t* field, double value);

int model_set_timestamp(mfield_t* field, tm_t* value);
int model_set_timestamptz(mfield_t* field, tm_t* value);
int model_set_date(mfield_t* field, tm_t* value);
int model_set_time(mfield_t* field, tm_t* value);
int model_set_timetz(mfield_t* field, tm_t* value);

int model_set_json(mfield_t* field, jsondoc_t* value);

int model_set_binary(mfield_t* field, const char* value, const size_t size);
int model_set_varchar(mfield_t* field, const char* value);
int model_set_char(mfield_t* field, const char* value);
int model_set_text(mfield_t* field, const char* value);
int model_set_enum(mfield_t* field, const char* value);




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




str_t* model_bool_to_str(mfield_t* field);
str_t* model_smallint_to_str(mfield_t* field);
str_t* model_int_to_str(mfield_t* field);
str_t* model_bigint_to_str(mfield_t* field);
str_t* model_float_to_str(mfield_t* field);
str_t* model_double_to_str(mfield_t* field);
str_t* model_decimal_to_str(mfield_t* field);
str_t* model_money_to_str(mfield_t* field);

str_t* model_timestamp_to_str(mfield_t* field);
str_t* model_timestamptz_to_str(mfield_t* field);
str_t* model_date_to_str(mfield_t* field);
str_t* model_time_to_str(mfield_t* field);
str_t* model_timetz_to_str(mfield_t* field);

str_t* model_json_to_str(mfield_t* field);



void model_params_clear(void* params, const size_t size);
void model_params_free(void* params, const size_t size);

#endif
