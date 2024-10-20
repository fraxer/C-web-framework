#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#include "db.h"
#include "log.h"
#include "model.h"

static void __model_value_clear(mvalue_t* value, mtype_e type);
static void __model_value_free(mvalue_t* value, mtype_e type);
static int __model_fill(const int row, const int fields_count, mfield_t* first_field, dbresult_t* result);
static void* __modelview_fill(void*(create_instance)(void), dbresult_t* result);
static array_t* __modelview_fill_array(void*(create_instance)(void), dbresult_t* result);
static int __model_set_binary(mfield_t* field, const char* value, const size_t size);
static int __model_set_date(mfield_t* field, tm_t* value);
static jsontok_t* __model_json_create_object(void* arg, jsondoc_t* doc);
static mfield_t __model_tmpfield_create(mfield_t* source_field);
static int __model_field_is_primary_and_not_dirty(model_t* model, mfield_t* field);

tm_t* tm_create(tm_t* time) {
    tm_t* tm = malloc(sizeof * tm);
    if (tm == NULL) return NULL;

    if (time == NULL)
        memset(tm, 0, sizeof * tm);
    else
        memcpy(tm, time, sizeof * tm);

    return tm;
}

void* model_get(const char* dbid, void*(create_instance)(void), array_t* params) {
    if (create_instance == NULL) return NULL;

    model_t* model = create_instance();
    if (model == NULL) return NULL;

    void* res = NULL;

    dbinstance_t* dbinst = dbinstance(dbid);
    if (dbinst == NULL) return 0;

    str_t* fields = str_create_empty();
    if (fields == NULL) return 0;

    dbconnection_t* conn = dbinst->connection;

    str_t* where_params = str_create_empty();
    if (where_params == NULL) goto failed;

    for (int i = 0; i < model->fields_count(model); i++) {
        mfield_t* field = model->first_field(model) + i;
        if (field == NULL)
            goto failed;

        if (i > 0)
            str_appendc(fields, ',');

        str_t* field_name = str_create(field->name, strlen(field->name));
        conn->escape_identifier(conn, field_name);

        str_append(fields, str_get(field_name), str_size(field_name));
        str_free(field_name);
    }

    for (size_t i = 0; i < array_size(params); i++) {
        mfield_t* param = array_get(params, i);

        if (i > 0)
            str_append(where_params, " AND ", 5);

        str_append(where_params, param->name, strlen(param->name));
        str_appendc(where_params, '=');

        str_t* value = model_field_to_string(param);
        if (value == NULL) goto failed;

        str_t* escaped_value = conn->escape_string(conn, value);
        if (escaped_value == NULL) goto failed;

        str_append(where_params, str_get(escaped_value), str_size(escaped_value));
        str_free(escaped_value);
    }

    dbresult_t* result = dbqueryf(dbinst,
        "SELECT "
            "%s "
        "FROM "
            "%s "
        "WHERE "
            "%s "
        "LIMIT 1 "
        ,
        str_get(fields),
        model->table(model),
        str_get(where_params)
    );

    if (!dbresult_ok(result))
        goto failed;

    if (dbresult_query_rows(result) == 0)
        goto failed;

    const int row = 0;
    if (!__model_fill(row, model->fields_count(model), model->first_field(model), result))
        goto failed;

    res = model;

    failed:

    dbinstance_free(dbinst);
    str_free(fields);
    str_free(where_params);
    dbresult_free(result);

    return res;
}

int model_create(const char* dbid, void* arg) {
    if (dbid == NULL) return 0;
    if (arg == NULL) return 0;

    int res = 0;
    model_t* model = arg;
    if (model->fields_count(model) == 0)
        return 0;

    array_t* arr = array_create();
    if (arr == NULL) return 0;

    for (int i = 0; i < model->fields_count(model); i++) {
        mfield_t* field = model->first_field(model) + i;
        if (__model_field_is_primary_and_not_dirty(model, field))
            continue;

        array_push_back(arr, array_create_pointer(field, array_nocopy, array_nofree));
    }

    dbresult_t* result = NULL;
    dbinstance_t* dbinst = dbinstance(dbid);
    if (dbinst == NULL) goto failed;

    result = dbinsert(dbinst, model->table(model), arr);
    if (!dbresult_ok(result)) {
        // log_error("model_create: Error insert model %s\n", dbinst->error);
        goto failed;
    }

    res = 1;

    failed:

    array_free(arr);
    dbinstance_free(dbinst);
    dbresult_free(result);

    return res;
}

int model_update(const char* dbid, void* arg) {
    if (arg == NULL) return 0;

    model_t* model = arg;
    int res = 0;

    dbinstance_t* dbinst = dbinstance(dbid);
    if (dbinst == NULL) return 0;

    dbconnection_t* conn = dbinst->connection;

    mfield_t* first_field = model->first_field(model);
    const char** unique_fields = model->primary_key(model);
    str_t* set_params = str_create_empty();
    if (set_params == NULL) return 0;

    str_t* where_params = str_create_empty();
    if (where_params == NULL) goto failed;

    for (int i = 0, iter_set = 0, iter_where = 0; i < model->fields_count(model); i++) {
        mfield_t* field = first_field + i;

        for (int j = 0; j < model->primary_key_count(model); j++) {
            if (strcmp(unique_fields[j], field->name) != 0)
                continue;

            mfield_t tmpfield = __model_tmpfield_create(field);
            if (field->dirty) {
                tmpfield.value = field->oldvalue;
                tmpfield.oldvalue = field->value;
            }

            str_t* fieldstr = model_field_to_string(&tmpfield);
            if (fieldstr == NULL)
                goto failed;

            if (iter_where > 0)
                str_append(where_params, " AND ", 5);

            str_append(where_params, field->name, strlen(field->name));
            str_appendc(where_params, '=');

            str_t* escaped_fieldstr = conn->escape_string(conn, fieldstr);
            if (escaped_fieldstr == NULL) goto failed;

            str_append(where_params, str_get(escaped_fieldstr), str_size(escaped_fieldstr));
            str_free(escaped_fieldstr);

            iter_where++;

            break;
        }

        if (!field->dirty)
            continue;

        str_t* fieldstr = model_field_to_string(field);
        if (fieldstr == NULL)
            continue;

        if (iter_set > 0)
            str_appendc(set_params, ',');

        str_append(set_params, field->name, strlen(field->name));
        str_appendc(set_params, '=');

        str_t* escaped_fieldstr = conn->escape_string(conn, fieldstr);
        if (escaped_fieldstr == NULL) goto failed;

        str_append(set_params, str_get(escaped_fieldstr), str_size(escaped_fieldstr));
        str_free(escaped_fieldstr);

        iter_set++;
    }

    dbresult_t* result = dbqueryf(dbinst,
        "UPDATE "
            "%s "
        "SET "
            "%s "
        "WHERE "
            "%s "
        ,
        model->table(model),
        str_get(set_params),
        str_get(where_params)
    );

    if (!dbresult_ok(result))
        goto failed;

    res = 1;

    for (int i = 0; i < model->fields_count(model); i++) {
        mfield_t* field = first_field + i;
        field->dirty = 0;
        __model_value_clear(&field->oldvalue, field->type);
    }

    failed:

    dbinstance_free(dbinst);
    str_free(set_params);
    str_free(where_params);
    dbresult_free(result);

    return res;
}

int model_delete(const char* dbid, void* arg) {
    if (dbid == NULL) return 0;
    if (arg == NULL) return 0;

    int res = 0;
    model_t* model = arg;

    if (model->fields_count(model) == 0) return 0;

    dbinstance_t* dbinst = dbinstance(dbid);
    if (dbinst == NULL) return 0;

    dbconnection_t* conn = dbinst->connection;

    mfield_t* vfield = model->first_field(arg);
    const char** vunique = model->primary_key(arg);
    str_t* where_params = str_create_empty();
    if (where_params == NULL) return 0;

    for (int i = 0, iter_where = 0; i < model->fields_count(model); i++) {
        mfield_t* field = vfield + i;

        for (int j = 0; j < model->primary_key_count(model); j++) {
            if (strcmp(vunique[j], field->name) != 0)
                continue;

            str_t* fieldstr = model_field_to_string(field);
            if (fieldstr == NULL)
                goto failed;

            if (iter_where > 0)
                str_append(where_params, " AND ", 5);

            str_append(where_params, field->name, strlen(field->name));
            str_appendc(where_params, '=');

            str_t* escaped_fieldstr = conn->escape_string(conn, fieldstr);
            if (escaped_fieldstr == NULL) goto failed;

            str_append(where_params, str_get(escaped_fieldstr), str_size(escaped_fieldstr));
            str_free(escaped_fieldstr);

            iter_where++;

            break;
        }
    }

    dbresult_t* result = dbqueryf(dbinst,
        "DELETE FROM "
            "%s "
        "WHERE "
            "%s "
        ,
        model->table(model),
        str_get(where_params)
    );

    if (!dbresult_ok(result))
        goto failed;

    res = 1;

    failed:

    dbinstance_free(dbinst);
    str_free(where_params);
    dbresult_free(result);

    return res;
}

void* model_one(const char* dbid, void*(create_instance)(void), const char* format, array_t* params) {
    dbinstance_t* dbinst = dbinstance(dbid);
    if (dbinst == NULL) return NULL;

    modelview_t* model = NULL;

    dbresult_t* result = dbquery(dbinst, format, params);
    dbinstance_free(dbinst);

    if (!dbresult_ok(result))
        goto failed;

    if (dbresult_query_rows(result) == 0)
        goto failed;

    model = __modelview_fill(create_instance, result);
    if (model == NULL)
        goto failed;

    failed:

    dbresult_free(result);

    return model;
}

array_t* model_list(const char* dbid, void*(create_instance)(void), const char* format, array_t* params) {
    dbinstance_t* dbinst = dbinstance(dbid);
    if (dbinst == NULL) return NULL;

    array_t* array = NULL;

    dbresult_t* result = dbquery(dbinst, format, params);
    dbinstance_free(dbinst);

    if (!dbresult_ok(result))
        goto failed;

    if (dbresult_query_rows(result) == 0)
        goto failed;

    array = __modelview_fill_array(create_instance, result);

    failed:

    dbresult_free(result);

    return array;
}

int model_execute(const char* dbid, const char* format, array_t* params) {
    int res = 0;

    dbinstance_t* dbinst = dbinstance(dbid);
    if (dbinst == NULL) return 0;

    dbresult_t* result = dbquery(dbinst, format, params);
    dbinstance_free(dbinst);

    if (!dbresult_ok(result))
        goto failed;

    res = 1;

    failed:

    dbresult_free(result);

    return res;
}

jsontok_t* model_to_json(void* arg, jsondoc_t* document) {
    return __model_json_create_object(arg, document);
}

char* model_stringify(void* arg) {
    if (arg == NULL) return NULL;

    jsondoc_t* doc = json_init();
    if (!doc) return NULL;

    jsontok_t* object = __model_json_create_object(arg, doc);
    if (object == NULL) {
        json_free(doc);
        return NULL;
    }

    char* data = json_stringify_detach(doc);

    json_free(doc);

    return data;
}

char* model_list_stringify(array_t* array) {
    if (array == NULL) return NULL;

    jsondoc_t* doc = json_init();
    if (!doc) return NULL;

    jsontok_t* json_array = json_create_array(doc);
    char* data = NULL;

    for (size_t i = 0; i < array_size(array); i++) {
        avalue_t* item = &array->elements[i];
        if (item->type != ARRAY_POINTER) goto failed;

        model_t* model = item->_pointer;
        if (model == NULL) goto failed;

        json_array_append(json_array, model_to_json(model, doc));
    }

    data = json_stringify_detach(doc);

    failed:

    json_free(doc);

    return data;
}

void model_free(void* arg) {
    model_t* model = arg;
    if (model == NULL) return;

    for (int i = 0; i < model->fields_count(model); i++) {
        mfield_t* field = model->first_field(model) + i;
        __model_value_free(&field->value, field->type);
        __model_value_free(&field->oldvalue, field->type);
    }

    free(model);
}

short model_bool(mfield_t* field) {
    if (field == NULL) return 0;
    if (field->type != MODEL_BOOL) return 0;

    return field->value._short;
}

short model_smallint(mfield_t* field) {
    if (field == NULL) return 0;
    if (field->type != MODEL_SMALLINT) return 0;

    return field->value._short;
}

int model_int(mfield_t* field) {
    if (field == NULL) return 0;
    if (field->type != MODEL_INT) return 0;

    return field->value._int;
}

long long int model_bigint(mfield_t* field) {
    if (field == NULL) return 0;
    if (field->type != MODEL_BIGINT) return 0;

    return field->value._bigint;
}

float model_float(mfield_t* field) {
    if (field == NULL) return 0.0;
    if (field->type != MODEL_FLOAT) return 0.0;

    return field->value._float;
}

double model_double(mfield_t* field) {
    if (field == NULL) return 0.0;
    if (field->type != MODEL_DOUBLE) return 0.0;

    return field->value._double;
}

long double model_decimal(mfield_t* field) {
    if (field == NULL) return 0.0;
    if (field->type != MODEL_DECIMAL) return 0.0;

    return field->value._ldouble;
}

double model_money(mfield_t* field) {
    if (field == NULL) return 0.0;
    if (field->type != MODEL_MONEY) return 0.0;

    return field->value._double;
}

tm_t* model_timestamp(mfield_t* field) {
    if (field == NULL) return NULL;
    if (field->type != MODEL_TIMESTAMP) return NULL;

    return field->value._tm;
}

tm_t* model_timestamptz(mfield_t* field) {
    if (field == NULL) return NULL;
    if (field->type != MODEL_TIMESTAMPTZ) return NULL;

    return field->value._tm;
}

tm_t* model_date(mfield_t* field) {
    if (field == NULL) return NULL;
    if (field->type != MODEL_DATE) return NULL;

    return field->value._tm;
}

tm_t* model_time(mfield_t* field) {
    if (field == NULL) return NULL;
    if (field->type != MODEL_TIME) return NULL;

    return field->value._tm;
}

tm_t* model_timetz(mfield_t* field) {
    if (field == NULL) return NULL;
    if (field->type != MODEL_TIMETZ) return NULL;

    return field->value._tm;
}

jsondoc_t* model_json(mfield_t* field) {
    if (field == NULL) return NULL;
    if (field->type != MODEL_JSON) return NULL;

    return field->value._jsondoc;
}

str_t* model_binary(mfield_t* field) {
    if (field == NULL) return NULL;
    if (field->type != MODEL_BINARY) return NULL;

    return field->value._string;
}

str_t* model_varchar(mfield_t* field) {
    if (field == NULL) return NULL;
    if (field->type != MODEL_VARCHAR) return NULL;

    return field->value._string;
}

str_t* model_char(mfield_t* field) {
    if (field == NULL) return NULL;
    if (field->type != MODEL_CHAR) return NULL;

    return field->value._string;
}

str_t* model_text(mfield_t* field) {
    if (field == NULL) return NULL;
    if (field->type != MODEL_TEXT) return NULL;

    return field->value._string;
}

str_t* model_enum(mfield_t* field) {
    if (field == NULL) return NULL;
    if (field->type != MODEL_ENUM) return NULL;

    return field->value._string;
}

array_t* model_array(mfield_t* field) {
    if (field == NULL) return NULL;
    if (field->type != MODEL_ARRAY) return NULL;

    return field->value._array;
}

int model_set_bool(mfield_t* field, short value) {
    if (field == NULL) return 0;
    if (field->type != MODEL_BOOL) return 0;

    if (!field->dirty) {
        field->oldvalue._short = field->value._short;
        field->dirty = 1;
    }

    field->value._short = value;

    str_clear(field->value._string);

    return 1;
}

int model_set_smallint(mfield_t* field, short value) {
    if (field == NULL) return 0;
    if (field->type != MODEL_SMALLINT) return 0;

    if (!field->dirty) {
        field->oldvalue._short = field->value._short;
        field->dirty = 1;
    }

    field->value._short = value;

    str_clear(field->value._string);

    return 1;
}

int model_set_int(mfield_t* field, int value) {
    if (field == NULL) return 0;
    if (field->type != MODEL_INT) return 0;

    if (!field->dirty) {
        field->oldvalue._int = field->value._int;
        field->dirty = 1;
    }

    field->value._int = value;

    str_clear(field->value._string);

    return 1;
}

int model_set_bigint(mfield_t* field, long long int value) {
    if (field == NULL) return 0;
    if (field->type != MODEL_BIGINT) return 0;

    if (!field->dirty) {
        field->oldvalue._bigint = field->value._bigint;
        field->dirty = 1;
    }

    field->value._bigint = value;

    str_clear(field->value._string);

    return 1;
}

int model_set_float(mfield_t* field, float value) {
    if (field == NULL) return 0;
    if (field->type != MODEL_FLOAT) return 0;

    if (!field->dirty) {
        field->oldvalue._float = field->value._float;
        field->dirty = 1;
    }

    field->value._float = value;

    str_clear(field->value._string);

    return 1;
}

int model_set_double(mfield_t* field, double value) {
    if (field == NULL) return 0;
    if (field->type != MODEL_DOUBLE) return 0;

    if (!field->dirty) {
        field->oldvalue._double = field->value._double;
        field->dirty = 1;
    }

    field->value._double = value;

    str_clear(field->value._string);

    return 1;
}

int model_set_decimal(mfield_t* field, long double value) {
    if (field == NULL) return 0;
    if (field->type != MODEL_DECIMAL) return 0;

    if (!field->dirty) {
        field->oldvalue._ldouble = field->value._ldouble;
        field->dirty = 1;
    }

    field->value._ldouble = value;

    str_clear(field->value._string);

    return 1;
}

int model_set_money(mfield_t* field, double value) {
    if (field == NULL) return 0;
    if (field->type != MODEL_MONEY) return 0;

    if (!field->dirty) {
        field->oldvalue._double = field->value._double;
        field->dirty = 1;
    }

    field->value._double = value;

    str_clear(field->value._string);

    return 1;
}

int model_set_timestamp(mfield_t* field, tm_t* value) {
    if (field == NULL) return 0;
    if (field->type != MODEL_TIMESTAMP) return 0;

    return __model_set_date(field, value);
}

int model_set_timestamptz(mfield_t* field, tm_t* value) {
    if (field == NULL) return 0;
    if (field->type != MODEL_TIMESTAMPTZ) return 0;

    return __model_set_date(field, value);
}

int model_set_date(mfield_t* field, tm_t* value) {
    if (field == NULL) return 0;
    if (field->type != MODEL_DATE) return 0;

    return __model_set_date(field, value);
}

int model_set_time(mfield_t* field, tm_t* value) {
    if (field == NULL) return 0;
    if (field->type != MODEL_TIME) return 0;

    return __model_set_date(field, value);
}

int model_set_timetz(mfield_t* field, tm_t* value) {
    if (field == NULL) return 0;
    if (field->type != MODEL_TIMETZ) return 0;

    return __model_set_date(field, value);
}

int model_set_json(mfield_t* field, jsondoc_t* value) {
    if (field == NULL) return 0;
    if (field->type != MODEL_JSON) return 0;

    if (!field->dirty) {
        field->oldvalue._jsondoc = field->value._jsondoc;
        field->value._jsondoc = NULL;
        field->dirty = 1;
    }

    json_free(field->value._jsondoc);

    jsondoc_t* document = json_init();
    if (document == NULL) return 0;

    if (!json_copy(value, document)) {
        json_free(document);
        return 0;
    }

    field->value._jsondoc = document;

    str_clear(field->value._string);

    return 1;
}

int model_set_binary(mfield_t* field, const char* value, const size_t size) {
    return model_set_binary_from_str(field, value, size);
}

int model_set_varchar(mfield_t* field, const char* value) {
    return model_set_varchar_from_str(field, value, strlen(value));
}

int model_set_char(mfield_t* field, const char* value) {
    return model_set_char_from_str(field, value, strlen(value));
}

int model_set_text(mfield_t* field, const char* value) {
    return model_set_text_from_str(field, value, strlen(value));
}

int model_set_enum(mfield_t* field, const char* value) {
    return model_set_enum_from_str(field, value, strlen(value));
}

int model_set_array(mfield_t* field, array_t* value) {
    if (field == NULL) return 0;
    if (field->type != MODEL_ARRAY) return 0;
    if (value == NULL) return 0;

    if (!field->dirty) {
        field->oldvalue._array = field->value._array;
        field->value._array = NULL;
        field->dirty = 1;
    }

    field->value._array = value;

    str_clear(field->value._string);

    return 1;
}

int __model_set_binary(mfield_t* field, const char* value, const size_t size) {
    if (field == NULL) return 0;
    if (value == NULL) return 0;

    if (!field->dirty) {
        if (field->oldvalue._string == NULL)
            field->oldvalue._string = str_create_empty();

        if (field->oldvalue._string == NULL)
            return 0;

        str_move(field->value._string, field->oldvalue._string);
        field->dirty = 1;
    }

    if (!str_reset(field->value._string))
        return 0;

    return str_assign(field->value._string, value, size);
}

int __model_set_date(mfield_t* field, tm_t* value) {
    if (field == NULL) return 0;
    if (value == NULL) return 0;

    if (!field->dirty) {
        field->oldvalue._tm = field->value._tm;
        field->value._tm = NULL;
        field->dirty = 1;
    }

    if (field->value._tm != NULL)
        free(field->value._tm);

    field->value._tm = tm_create(value);

    str_clear(field->value._string);

    return 1;
}

str_t* model_field_to_string(mfield_t* field) {
    if (field == NULL) return NULL;

    switch (field->type) {
    case MODEL_BOOL:
        return model_bool_to_str(field);
    case MODEL_SMALLINT:
        return model_smallint_to_str(field);
    case MODEL_INT:
        return model_int_to_str(field);
    case MODEL_BIGINT:
        return model_bigint_to_str(field);
    case MODEL_FLOAT:
        return model_float_to_str(field);
    case MODEL_DOUBLE:
        return model_double_to_str(field);
    case MODEL_DECIMAL:
        return model_decimal_to_str(field);
    case MODEL_MONEY:
        return model_money_to_str(field);
    case MODEL_DATE:
        return model_date_to_str(field);
    case MODEL_TIME:
        return model_time_to_str(field);
    case MODEL_TIMETZ:
        return model_timetz_to_str(field);
    case MODEL_TIMESTAMP:
        return model_timestamp_to_str(field);
    case MODEL_TIMESTAMPTZ:
        return model_timestamptz_to_str(field);
    case MODEL_JSON:
        return model_json_to_str(field);
    case MODEL_BINARY:
        return model_binary(field);
    case MODEL_VARCHAR:
        return model_varchar(field);
    case MODEL_CHAR:
        return model_char(field);
    case MODEL_TEXT:
        return model_text(field);
    case MODEL_ENUM:
        return model_enum(field);
    case MODEL_ARRAY:
        return model_array_to_str(field);
    }

    return NULL;
}

void __model_value_clear(mvalue_t* value, mtype_e type) {
    if (value == NULL) return;

    switch (type) {
    case MODEL_BOOL:
    case MODEL_SMALLINT:
    case MODEL_INT:
    case MODEL_BIGINT:
    case MODEL_FLOAT:
    case MODEL_DOUBLE:
    case MODEL_DECIMAL:
    case MODEL_MONEY:
        value->_short = 0;
        break;

    case MODEL_DATE:
    case MODEL_TIME:
    case MODEL_TIMETZ:
    case MODEL_TIMESTAMP:
    case MODEL_TIMESTAMPTZ:
        if (value->_tm != NULL)
            memset(value->_tm, 0, sizeof * value->_tm);
        break;

    case MODEL_JSON:
        json_clear(value->_jsondoc);
        break;

    case MODEL_ARRAY:
        break;

    default:
        break;
    }

    str_clear(value->_string);
}

void __model_value_free(mvalue_t* value, mtype_e type) {
    if (value == NULL) return;

    switch (type) {
    case MODEL_BOOL:
    case MODEL_SMALLINT:
    case MODEL_INT:
    case MODEL_BIGINT:
    case MODEL_FLOAT:
    case MODEL_DOUBLE:
    case MODEL_DECIMAL:
    case MODEL_MONEY:
        value->_short = 0;
        break;

    case MODEL_DATE:
    case MODEL_TIME:
    case MODEL_TIMETZ:
    case MODEL_TIMESTAMP:
    case MODEL_TIMESTAMPTZ:
        if (value->_tm != NULL)
            free(value->_tm);
        break;

    case MODEL_JSON:
        json_free(value->_jsondoc);
        break;

    case MODEL_ENUM:
        enums_free(value->_enum);
        break;

    case MODEL_ARRAY:
        break;

    default:
        break;
    }

    str_free(value->_string);
}

int __model_fill(const int row, const int fields_count, mfield_t* first_field, dbresult_t* result) {
    for (int col = 0; col < dbresult_query_cols(result); col++) {
        const db_table_cell_t* field = dbresult_cell(result, row, col);

        for (int i = 0; i < fields_count; i++) {
            mfield_t* modelfield = first_field + i;
            if (modelfield == NULL)
                return 0;

            if (strcmp(modelfield->name, result->current->fields[col].value) == 0) {
                switch (modelfield->type) {
                case MODEL_BOOL:
                    if (!model_set_bool_from_str(modelfield, field->value))
                        return 0;
                    break;
                case MODEL_SMALLINT:
                    if (!model_set_smallint_from_str(modelfield, field->value))
                        return 0;
                    break;
                case MODEL_INT:
                    if (!model_set_int_from_str(modelfield, field->value))
                        return 0;
                    break;
                case MODEL_BIGINT:
                    if (!model_set_bigint_from_str(modelfield, field->value))
                        return 0;
                    break;
                case MODEL_FLOAT:
                    if (!model_set_float_from_str(modelfield, field->value))
                        return 0;
                    break;
                case MODEL_DOUBLE:
                    if (!model_set_double_from_str(modelfield, field->value))
                        return 0;
                    break;
                case MODEL_DECIMAL:
                    if (!model_set_decimal_from_str(modelfield, field->value))
                        return 0;
                    break;
                case MODEL_MONEY:
                    if (!model_set_money_from_str(modelfield, field->value))
                        return 0;
                    break;
                case MODEL_DATE:
                    if (!model_set_date_from_str(modelfield, field->value))
                        return 0;
                    break;
                case MODEL_TIME:
                    if (!model_set_time_from_str(modelfield, field->value))
                        return 0;
                    break;
                case MODEL_TIMETZ:
                    if (!model_set_timetz_from_str(modelfield, field->value))
                        return 0;
                    break;
                case MODEL_TIMESTAMP:
                    if (!model_set_timestamp_from_str(modelfield, field->value))
                        return 0;
                    break;
                case MODEL_TIMESTAMPTZ:
                    if (!model_set_timestamptz_from_str(modelfield, field->value))
                        return 0;
                    break;
                case MODEL_JSON:
                    if (!model_set_json_from_str(modelfield, field->value))
                        return 0;
                    break;
                case MODEL_BINARY:
                    if (!model_set_binary_from_str(modelfield, field->value, field->length))
                        return 0;
                    break;
                case MODEL_VARCHAR:
                    if (!model_set_varchar_from_str(modelfield, field->value, field->length))
                        return 0;
                    break;
                case MODEL_CHAR:
                    if (!model_set_char_from_str(modelfield, field->value, field->length))
                        return 0;
                    break;
                case MODEL_TEXT:
                    if (!model_set_text_from_str(modelfield, field->value, field->length))
                        return 0;
                    break;
                case MODEL_ENUM:
                    if (!model_set_enum_from_str(modelfield, field->value, field->length))
                        return 0;
                    break;
                case MODEL_ARRAY:
                    if (!model_set_array_from_str(modelfield, field->value))
                        return 0;
                    break;
                default:
                    return 0;
                }

                modelfield->dirty = 0;
                
                break;
            }
        }
    }

    return 1;
}

void* __modelview_fill(void*(create_instance)(void), dbresult_t* result) {
    if (result == NULL) return NULL;

    modelview_t* model = create_instance();
    if (model == NULL)
        return NULL;

    const int row = 0;
    if (!__model_fill(row, model->fields_count(model), model->first_field(model), result)) {
        model_free(model);
        return NULL;
    }

    return model;
}

array_t* __modelview_fill_array(void*(create_instance)(void), dbresult_t* result) {
    if (result == NULL) return 0;

    array_t* array = array_create();
    if (array == NULL)
        return NULL;

    for (int row = 0; row < dbresult_query_rows(result); row++) {
        modelview_t* model = create_instance();
        if (model == NULL)
            goto failed;

        if (!__model_fill(row, model->fields_count(model), model->first_field(model), result))
            goto failed;

        array_push_back(array, array_create_pointer(model, array_nocopy, model_free));
    }

    failed:

    return array;
}

int model_set_bool_from_str(mfield_t* field, const char* value) {
    if (field == NULL) return 0;
    if (field->type != MODEL_BOOL) return 0;

    if (strcmp(value, "1") == 0 || strcmp(value, "true") == 0 || strcmp(value, "t") == 0)
        return model_set_bool(field, 1);
    else if (strcmp(value, "0") == 0 || strcmp(value, "false") == 0 || strcmp(value, "f") == 0)
        return model_set_bool(field, 0);

    return 0;
}

int model_set_smallint_from_str(mfield_t* field, const char* value) {
    return model_set_smallint(field, (short)atoi(value));
}

int model_set_int_from_str(mfield_t* field, const char* value) {
    return model_set_int(field, atoi(value));
}

int model_set_bigint_from_str(mfield_t* field, const char* value) {
    return model_set_bigint(field, atoll(value));
}

int model_set_float_from_str(mfield_t* field, const char* value) {
    return model_set_float(field, (float)atof(value));
}

int model_set_double_from_str(mfield_t* field, const char* value) {
    return model_set_double(field, atof(value));
}

int model_set_decimal_from_str(mfield_t* field, const char* value) {
    return model_set_decimal(field, strtold(value, NULL));
}

int model_set_money_from_str(mfield_t* field, const char* value) {
    return model_set_money(field, atof(value));
}

int model_set_timestamp_from_str(mfield_t* field, const char* value) {
    if (field == NULL) return 0;
    if (field->type != MODEL_TIMESTAMP) return 0;

    memset(field->value._tm, 0, sizeof(tm_t));

    if (strlen(value) > 0 && strptime(value, "%Y-%m-%d %H:%M:%S", field->value._tm) == NULL)
        return 0;

    return model_set_timestamp(field, field->value._tm);
}

int model_set_timestamptz_from_str(mfield_t* field, const char* value) {
    if (field == NULL) return 0;
    if (field->type != MODEL_TIMESTAMPTZ) return 0;

    memset(field->value._tm, 0, sizeof(tm_t));

    if (strlen(value) > 0 && strptime(value, "%Y-%m-%d %H:%M:%S%z", field->value._tm) == NULL)
        return 0;

    return model_set_timestamptz(field, field->value._tm);
}

int model_set_date_from_str(mfield_t* field, const char* value) {
    if (field == NULL) return 0;
    if (field->type != MODEL_DATE) return 0;

    memset(field->value._tm, 0, sizeof(tm_t));

    if (strlen(value) > 0 && strptime(value, "%Y-%m-%d %H:%M:%S", field->value._tm) == NULL)
        return 0;

    return model_set_date(field, field->value._tm);
}

int model_set_time_from_str(mfield_t* field, const char* value) {
    if (field == NULL) return 0;
    if (field->type != MODEL_TIME) return 0;

    memset(field->value._tm, 0, sizeof(tm_t));

    if (strlen(value) > 0 && strptime(value, "%H:%M:%S", field->value._tm) == NULL)
        return 0;

    return model_set_time(field, field->value._tm);
}

int model_set_timetz_from_str(mfield_t* field, const char* value) {
    if (field == NULL) return 0;
    if (field->type != MODEL_TIMETZ) return 0;

    memset(field->value._tm, 0, sizeof(tm_t));

    if (strlen(value) > 0 && strptime(value, "%H:%M:%S%z", field->value._tm) == NULL)
        return 0;

    return model_set_timetz(field, field->value._tm);
}

int model_set_json_from_str(mfield_t* field, const char* value) {
    if (field == NULL) return 0;
    if (field->type != MODEL_JSON) return 0;

    jsondoc_t* document = json_create(value);
    if (document == NULL) return 0;

    field->value._jsondoc = document;

    return 1;
}

int model_set_binary_from_str(mfield_t* field, const char* value, size_t size) {
    if (field == NULL) return 0;
    if (field->type != MODEL_BINARY) return 0;

    return __model_set_binary(field, value, size);
}

int model_set_varchar_from_str(mfield_t* field, const char* value, size_t size) {
    if (field == NULL) return 0;
    if (field->type != MODEL_VARCHAR) return 0;

    return __model_set_binary(field, value, size);
}

int model_set_char_from_str(mfield_t* field, const char* value, size_t size) {
    if (field == NULL) return 0;
    if (field->type != MODEL_CHAR) return 0;

    return __model_set_binary(field, value, size);
}

int model_set_text_from_str(mfield_t* field, const char* value, size_t size) {
    if (field == NULL) return 0;
    if (field->type != MODEL_TEXT) return 0;

    return __model_set_binary(field, value, size);
}

int model_set_enum_from_str(mfield_t* field, const char* value, size_t size) {
    if (field == NULL) return 0;
    if (field->type != MODEL_ENUM) return 0;

    if (size == 0)
        return __model_set_binary(field, value, size);

    for (short i = 0; i < field->value._enum->count; i++)
        if (strcmp(field->value._enum->values[i], value) == 0)
            return __model_set_binary(field, value, size);

    return 0;
}

int model_set_array_from_str(mfield_t* field, const char* value) {
    jsondoc_t* document = json_create(value);
    if (document == NULL) return 0;

    const jsontok_t* token_array = json_root(document);
    if (token_array == NULL) {
        json_free(document);
        return 0;
    }
    if (!json_is_array(token_array)) {
        json_free(document);
        return 0;
    }

    array_t* array = array_create();
    if (array == NULL) {
        json_free(document);
        return 0;
    }

    for (jsonit_t it_array = json_init_it(token_array); !json_end_it(&it_array); json_next_it(&it_array)) {
        jsontok_t* token_value = json_it_value(&it_array);
        if (json_is_string(token_value)) {
            array_push_back(array, array_create_string(json_string(token_value)));
        } else if (json_is_int(token_value)) {
            array_push_back(array, array_create_double(json_double(token_value)));
        } else if (json_is_bool(token_value)) {
            array_push_back(array, array_create_double(json_bool(token_value)));
        } else if (json_is_null(token_value)) {
            array_push_back(array, array_create_double(0));
        }
        else {
            json_free(document);
            array_free(array);
            return 0;
        }
    }

    field->value._array = array;

    return 1;
}

str_t* model_bool_to_str(mfield_t* field) {
    if (field == NULL) return NULL;
    if (field->type != MODEL_BOOL) return NULL;

    char str[2] = {0};
    ssize_t size = snprintf(str, sizeof(str), "%d", model_bool(field));
    if (size < 0) return NULL;

    if (field->value._string == NULL)
        field->value._string = str_create(str, size);
    else
        str_assign(field->value._string, str, size);

    return field->value._string;
}

str_t* model_smallint_to_str(mfield_t* field) {
    if (field == NULL) return NULL;
    if (field->type != MODEL_SMALLINT) return NULL;

    char str[7] = {0};
    ssize_t size = snprintf(str, sizeof(str), "%d", model_smallint(field));
    if (size < 0) return NULL;

    if (field->value._string == NULL)
        field->value._string = str_create(str, size);
    else
        str_assign(field->value._string, str, size);

    return field->value._string;
}

str_t* model_int_to_str(mfield_t* field) {
    if (field == NULL) return NULL;
    if (field->type != MODEL_INT) return NULL;

    char str[12] = {0};
    ssize_t size = snprintf(str, sizeof(str), "%d", model_int(field));
    if (size < 0) return NULL;

    if (field->value._string == NULL)
        field->value._string = str_create(str, size);
    else
        str_assign(field->value._string, str, size);

    return field->value._string;
}

str_t* model_bigint_to_str(mfield_t* field) {
    if (field == NULL) return NULL;
    if (field->type != MODEL_BIGINT) return NULL;

    char str[21] = {0};
    ssize_t size = snprintf(str, sizeof(str), "%lld", model_bigint(field));
    if (size < 0) return NULL;

    if (field->value._string == NULL)
        field->value._string = str_create(str, size);
    else
        str_assign(field->value._string, str, size);

    return field->value._string;
}

str_t* model_float_to_str(mfield_t* field) {
    if (field == NULL) return NULL;
    if (field->type != MODEL_FLOAT) return NULL;

    char str[64] = {0};
    ssize_t size = snprintf(str, sizeof(str), "%.6f", model_float(field));
    if (size < 0) return NULL;

    if (field->value._string == NULL)
        field->value._string = str_create(str, size);
    else
        str_assign(field->value._string, str, size);

    return field->value._string;
}

str_t* model_double_to_str(mfield_t* field) {
    if (field == NULL) return NULL;
    if (field->type != MODEL_DOUBLE) return NULL;

    char str[375] = {0};
    ssize_t size = snprintf(str, sizeof(str), "%.12f", model_double(field));
    if (size < 0) return NULL;

    if (field->value._string == NULL)
        field->value._string = str_create(str, size);
    else
        str_assign(field->value._string, str, size);

    return field->value._string;
}

str_t* model_decimal_to_str(mfield_t* field) {
    if (field == NULL) return NULL;
    if (field->type != MODEL_DECIMAL) return NULL;

    char str[512] = {0};
    ssize_t size = snprintf(str, sizeof(str), "%.12Lf", model_decimal(field));
    if (size < 0) return NULL;

    if (field->value._string == NULL)
        field->value._string = str_create(str, size);
    else
        str_assign(field->value._string, str, size);

    return field->value._string;
}

str_t* model_money_to_str(mfield_t* field) {
    if (field == NULL) return NULL;
    if (field->type != MODEL_MONEY) return NULL;

    char str[375] = {0};
    ssize_t size = snprintf(str, sizeof(str), "%.12f", model_double(field));
    if (size < 0) return NULL;

    if (field->value._string == NULL)
        field->value._string = str_create(str, size);
    else
        str_assign(field->value._string, str, size);

    return field->value._string;
}

str_t* model_timestamp_to_str(mfield_t* field) {
    if (field == NULL) return NULL;
    if (field->type != MODEL_TIMESTAMP) return NULL;

    char value[32] = {0};
    const size_t size = strftime(value, sizeof(value), "%Y-%m-%d %H:%M:%S", field->value._tm);
    if (size == 0)
        return NULL;

    if (field->value._string == NULL)
        field->value._string = str_create(value, size);
    else
        str_assign(field->value._string, value, size);

    return field->value._string;
}

str_t* model_timestamptz_to_str(mfield_t* field) {
    if (field == NULL) return NULL;
    if (field->type != MODEL_TIMESTAMPTZ) return NULL;

    char value[32] = {0};
    const size_t size = strftime(value, sizeof(value), "%Y-%m-%d %H:%M:%S%z", field->value._tm);
    if (size == 0)
        return NULL;

    if (field->value._string == NULL)
        field->value._string = str_create(value, size);
    else
        str_assign(field->value._string, value, size);

    return field->value._string;
}

str_t* model_date_to_str(mfield_t* field) {
    if (field == NULL) return NULL;
    if (field->type != MODEL_DATE) return NULL;

    char value[32] = {0};
    const size_t size = strftime(value, sizeof(value), "%Y-%m-%d %H:%M:%S", field->value._tm);
    if (size == 0)
        return NULL;

    if (field->value._string == NULL)
        field->value._string = str_create(value, size);
    else
        str_assign(field->value._string, value, size);

    return field->value._string;
}

str_t* model_time_to_str(mfield_t* field) {
    if (field == NULL) return NULL;
    if (field->type != MODEL_TIME) return NULL;

    char value[10] = {0};
    const size_t size = strftime(value, sizeof(value), "%H:%M:%S", field->value._tm);
    if (size == 0)
        return NULL;

    if (field->value._string == NULL)
        field->value._string = str_create(value, size);
    else
        str_assign(field->value._string, value, size);

    return field->value._string;
}

str_t* model_timetz_to_str(mfield_t* field) {
    if (field == NULL) return NULL;
    if (field->type != MODEL_TIMETZ) return NULL;

    char value[20] = {0};
    const size_t size = strftime(value, sizeof(value), "%H:%M:%S%z", field->value._tm);
    if (size == 0)
        return NULL;

    if (field->value._string == NULL)
        field->value._string = str_create(value, size);
    else
        str_assign(field->value._string, value, size);

    return field->value._string;
}

str_t* model_json_to_str(mfield_t* field) {
    if (field == NULL) return NULL;
    if (field->type != MODEL_JSON) return NULL;

    char* data = json_stringify_detach(field->value._jsondoc);
    if (data == NULL) return NULL;

    if (field->value._string == NULL)
        field->value._string = str_create(data, strlen(data));
    else
        str_assign(field->value._string, data, strlen(data));

    free(data);

    return field->value._string;
}

str_t* model_array_to_str(mfield_t* field) {
    if (field == NULL) return NULL;
    if (field->type != MODEL_ARRAY) return NULL;

    array_t* array = field->value._array;
    str_t* string = str_create_empty();
    if (string == NULL) return NULL;

    str_appendc(string, '[');

    for (size_t i = 0; i < array_size(array); i++) {
        avalue_t* item = array_get(array, i);
        if (item->type == ARRAY_POINTER) continue;

        str_t* str = array_item_to_string(array, i);
        if (str == NULL) {
            str_free(string);
            return NULL;
        }

        if (i > 0)
            str_appendc(string, ',');

        str_modify_add_symbols_before(str, '\\', '"');
        str_appendc(string, '\"');
        str_append(string, str_get(str), str_size(str));
        str_appendc(string, '\"');
        str_free(str);
    }

    str_appendc(string, ']');

    if (field->value._string == NULL)
        field->value._string = string;
    else {
        str_assign(field->value._string, str_get(string), str_size(string));
        str_free(string);
    }

    return field->value._string;
}

void model_param_clear(void* field) {
    mfield_t* _field = field;

    __model_value_free(&_field->value, _field->type);
    __model_value_free(&_field->oldvalue, _field->type);
}

void model_params_clear(void* params, const size_t size) {
    const size_t count_fields = size / sizeof(mfield_t);

    mfield_t* fields = params;
    for (size_t i = 0; i < count_fields; i++)
        model_param_clear(fields + i);
}

void model_params_free(void* params, const size_t size) {
    model_params_clear(params, size);
    free(params);
}

jsontok_t* __model_json_create_object(void* arg, jsondoc_t* doc) {
    if (arg == NULL) return NULL;
    if (doc == NULL) return NULL;

    model_t* model = arg;

    int result = 0;
    jsontok_t* object = json_create_object(doc);
    if (object == NULL) return NULL;

    for (int i = 0; i < model->fields_count(model); i++) {
        mfield_t* field = model->first_field(model) + i;

        jsontok_t* token_value = NULL;

        switch (field->type) {
        case MODEL_BOOL:
            token_value = json_create_bool(doc, model_bool(field));
            break;
        case MODEL_SMALLINT:
            token_value = json_create_int(doc, model_smallint(field));
            break;
        case MODEL_INT:
            token_value = json_create_int(doc, model_int(field));
            break;
        case MODEL_BIGINT:
            token_value = json_create_int(doc, model_bigint(field));
            break;
        case MODEL_FLOAT:
            token_value = json_create_double(doc, model_float(field));
            break;
        case MODEL_DOUBLE:
            token_value = json_create_double(doc, model_double(field));
            break;
        case MODEL_DECIMAL:
            token_value = json_create_double(doc, model_decimal(field));
            break;
        case MODEL_MONEY:
            token_value = json_create_double(doc, model_double(field));
            break;
        case MODEL_DATE:
            token_value = json_create_string(doc, str_get(model_date_to_str(field)));
            break;
        case MODEL_TIME:
            token_value = json_create_string(doc, str_get(model_time_to_str(field)));
            break;
        case MODEL_TIMETZ:
            token_value = json_create_string(doc, str_get(model_timetz_to_str(field)));
            break;
        case MODEL_TIMESTAMP:
            token_value = json_create_string(doc, str_get(model_timestamp_to_str(field)));
            break;
        case MODEL_TIMESTAMPTZ:
            token_value = json_create_string(doc, str_get(model_timestamptz_to_str(field)));
            break;
        case MODEL_JSON:
            token_value = json_create_string(doc, str_get(model_json_to_str(field)));
            break;
        case MODEL_BINARY:
            token_value = json_create_string(doc, str_get(model_binary(field)));
            break;
        case MODEL_VARCHAR:
            token_value = json_create_string(doc, str_get(model_varchar(field)));
            break;
        case MODEL_CHAR:
            token_value = json_create_string(doc, str_get(model_char(field)));
            break;
        case MODEL_TEXT:
            token_value = json_create_string(doc, str_get(model_text(field)));
            break;
        case MODEL_ENUM:
            token_value = json_create_string(doc, str_get(model_enum(field)));
            break;
        case MODEL_ARRAY:
            token_value = json_create_string(doc, str_get(model_array_to_str(field)));
            break;
        }

        if (token_value == NULL) goto failed;

        json_object_set(object, field->name, token_value);
    }

    result = 1;

    failed:

    if (result == 0)
        return NULL;

    return object;
}

mfield_t __model_tmpfield_create(mfield_t* source_field) {
    mfield_t tmpfield = {
        .type = source_field->type,
        .dirty = source_field->dirty,
        .name = {0},
        .value = source_field->value,
        .oldvalue = source_field->oldvalue
    };

    strcpy(tmpfield.name, source_field->name);

    return tmpfield;
}

int __model_field_is_primary_and_not_dirty(model_t* model, mfield_t* field) {
    if (model == NULL) return 0;
    if (field == NULL) return 0;

    const char** primary_key = model->primary_key(model);
    for (int i = 0; i < model->primary_key_count(model); i++) {
        if (strcmp(primary_key[i], field->name) != 0)
            continue;

        if (!field->dirty)
            return 1;

        break;
    }
    
    return 0;
}
