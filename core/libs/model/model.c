#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "db.h"
#include "model.h"
#include "bufferdata.h"

int model_get(const char* dbid, void* arg, mfield_t* params, int params_count) {
    if (arg == NULL) return 0;

    model_t* model = arg;
    int res = 0;

    dbinstance_t dbinst = dbinstance(dbid);
    if (!dbinst.ok) return 0;

    char fields[2048] = {0};
    bufferdata_t where_params;
    bufferdata_init(&where_params);

    mfield_t* vfield = model->first_field(model);
    for (int i = 0; i < model->fields_count(model); i++) {
        mfield_t* field = vfield + i;
        if (field == NULL)
            goto failed;

        if (i > 0)
            strcat(fields, ",");

        strcat(fields, field->name);
    }

    for (int i = 0; i < params_count; i++) {
        mfield_t* param = params + i;

        if (i > 0) {
            bufferdata_push(&where_params, ' ');
            bufferdata_push(&where_params, 'A');
            bufferdata_push(&where_params, 'N');
            bufferdata_push(&where_params, 'D');
            bufferdata_push(&where_params, ' ');
        }

        for (size_t j = 0; j < strlen(param->name); j++)
            bufferdata_push(&where_params, param->name[j]);

        bufferdata_push(&where_params, '=');

        const char* value = mvalue_to_string(&param->value, param->type);
        if (value == NULL) goto failed;

        size_t value_length = strlen(value);

        bufferdata_push(&where_params, '\'');
        for (size_t j = 0; j < value_length; j++)
            bufferdata_push(&where_params, value[j]);

        bufferdata_push(&where_params, '\'');
    }

    bufferdata_complete(&where_params);

    dbresult_t result = dbquery(&dbinst,
        "SET ROLE slave_select;"
        "SELECT "
            "%s "
        "FROM "
            "%s "
        "WHERE "
            "%s "
        "LIMIT 1 "
        ,
        fields,
        model->table(model),
        bufferdata_get(&where_params)
    );

    if (!dbresult_ok(&result))
        goto failed;

    if (dbresult_query_rows(&result) == 0)
        goto failed;

    const int row = 0;
    for (int col = 0; col < dbresult_query_cols(&result); col++) {
        printf("%d %d %s\n", row, col, result.current->fields[col].value);

        const db_table_cell_t* field = dbresult_cell(&result, row, col);

        for (int i = 0; i < model->fields_count(model); i++) {
            mfield_t* modelfield = vfield + i;
            if (modelfield == NULL)
                goto failed;

            if (strcmp(modelfield->name, result.current->fields[col].value) == 0) {
                if (modelfield->type == MODEL_INT) {
                    model_set_int(modelfield, mstr_to_int(field->value));
                }
                else if (modelfield->type == MODEL_STRING) {
                    model_set_stringn(modelfield, field->value, field->length);
                }
                else {
                    goto failed;
                }

                modelfield->dirty = 0;
                break;
            }
        }
    }

    res = 1;

    failed:

    bufferdata_clear(&where_params);
    dbresult_free(&result);

    return res;
}

int model_create(const char* dbid, void* arg) {
    if (dbid == NULL) return 0;
    if (arg == NULL) return 0;

    int res = 0;
    model_t* model = arg;

    if (model->fields_count(model) == 0) return 0;

    dbinstance_t dbinst = dbinstance(dbid);
    if (!dbinst.ok) return 0;

    char fields[2048] = {0};
    const char** primary_key = model->primary_key(model);
    bufferdata_t values;
    bufferdata_init(&values);

    mfield_t* vfield = model->first_field(model);
    if (vfield == NULL)
        goto failed;

    for (int i = 0, iter_set = 0; i < model->fields_count(model); i++) {
        mfield_t* field = vfield + i;
        int empty_primary_key = 0;
        for (int j = 0; j < model->primary_key_count(model); j++) {
            if (strcmp(primary_key[j], field->name) != 0)
                continue;

            if (field->type == MODEL_STRING) {
                if (field->value._string == NULL || field->value._length == 0)
                    empty_primary_key = 1;
            }
            else {
                if (field->value._int == 0)
                    empty_primary_key = 1;
            }

            break;
        }

        if (empty_primary_key)
            continue;

        if (iter_set > 0) {
            strcat(fields, ",");
            bufferdata_push(&values, ',');
        }

        const char* value = mvalue_to_string(&field->value, field->type);
        if (value == NULL) goto failed;

        strcat(fields, field->name);

        size_t value_length = strlen(value);

        bufferdata_push(&values, '\'');
        for (size_t j = 0; j < value_length; j++)
            bufferdata_push(&values, value[j]);

        bufferdata_push(&values, '\'');

        iter_set++;
    }

    bufferdata_complete(&values);

    dbresult_t result = dbquery(&dbinst,
        "SET ROLE slave_insert;"
        "INSERT INTO "
            "%s "
            "("
                "%s "
            ") "
        "VALUES "
            "("
                "%s "
            ")",
        model->table(model),
        fields,
        bufferdata_get(&values)
    );

    if (!dbresult_ok(&result))
        goto failed;

    res = 1;

    failed:

    bufferdata_clear(&values);
    dbresult_free(&result);

    return res;
}

int model_update(const char* dbid, void* arg) {
    if (arg == NULL) return 0;

    model_t* model = arg;
    int res = 0;

    dbinstance_t dbinst = dbinstance(dbid);
    if (!dbinst.ok) return 0;

    mfield_t* vfield = model->first_field(model);
    const char** vunique = model->primary_key(model);
    bufferdata_t set_params;
    bufferdata_init(&set_params);

    bufferdata_t where_params;
    bufferdata_init(&where_params);

    for (int i = 0, iter_set = 0, iter_where = 0; i < model->fields_count(model); i++) {
        mfield_t* field = vfield + i;

        for (int j = 0; j < model->primary_key_count(model); j++) {
            if (strcmp(vunique[j], field->name) != 0)
                continue;

            mvalue_t* v = &field->value;
            if (field->dirty)
                v = &field->oldvalue;

            const char* fieldvalue = mvalue_to_string(&field->value, field->type);
            if (fieldvalue == NULL)
                goto failed;

            if (iter_where > 0) {
                bufferdata_push(&where_params, ' ');
                bufferdata_push(&where_params, 'A');
                bufferdata_push(&where_params, 'N');
                bufferdata_push(&where_params, 'D');
                bufferdata_push(&where_params, ' ');
            }

            for (size_t k = 0; k < strlen(field->name); k++)
                bufferdata_push(&where_params, field->name[k]);

            bufferdata_push(&where_params, '=');
            bufferdata_push(&where_params, '\'');

            for (size_t j = 0; j < v->_length; j++)
                bufferdata_push(&where_params, fieldvalue[j]);

            bufferdata_push(&where_params, '\'');

            iter_where++;

            break;
        }

        if (!field->dirty)
            continue;

        const char* fieldvalue = mvalue_to_string(&field->value, field->type);
        if (fieldvalue == NULL)
            continue;

        if (iter_set > 0)
            bufferdata_push(&set_params, ',');

        for (size_t j = 0; j < strlen(field->name); j++)
            bufferdata_push(&set_params, field->name[j]);

        bufferdata_push(&set_params, '=');
        bufferdata_push(&set_params, '\'');

        for (size_t j = 0; j < field->value._length; j++)
            bufferdata_push(&set_params, fieldvalue[j]);

        bufferdata_push(&set_params, '\'');

        iter_set++;
    }

    bufferdata_complete(&set_params);
    bufferdata_complete(&where_params);

    dbresult_t result = dbquery(&dbinst,
        "SET ROLE slave_update;"
        "UPDATE "
            "%s "
        "SET "
            "%s "
        "WHERE "
            "%s "
        ,
        model->table(model),
        bufferdata_get(&set_params),
        bufferdata_get(&where_params)
    );

    if (!dbresult_ok(&result))
        goto failed;

    res = 1;

    for (int i = 0; i < model->fields_count(model); i++) {
        mfield_t* field = vfield + i;
        field->dirty = 0;
        mvalue_clear(&field->oldvalue);
    }

    failed:

    bufferdata_clear(&set_params);
    bufferdata_clear(&where_params);
    dbresult_free(&result);

    return res;
}

int model_delete(const char* dbid, void* arg) {
    if (dbid == NULL) return 0;
    if (arg == NULL) return 0;

    int res = 0;
    model_t* model = arg;

    if (model->fields_count(model) == 0) return 0;

    dbinstance_t dbinst = dbinstance(dbid);
    if (!dbinst.ok) return 0;

    mfield_t* vfield = model->first_field(arg);
    const char** vunique = model->primary_key(arg);
    bufferdata_t where_params;
    bufferdata_init(&where_params);

    for (int i = 0, iter_where = 0; i < model->fields_count(model); i++) {
        mfield_t* field = vfield + i;

        for (int j = 0; j < model->primary_key_count(model); j++) {
            if (strcmp(vunique[j], field->name) != 0)
                continue;

            mvalue_t* v = &field->value;
            if (field->dirty)
                v = &field->oldvalue;

            const char* fieldvalue = mvalue_to_string(&field->value, field->type);
            if (fieldvalue == NULL)
                goto failed;

            if (iter_where > 0) {
                bufferdata_push(&where_params, ' ');
                bufferdata_push(&where_params, 'A');
                bufferdata_push(&where_params, 'N');
                bufferdata_push(&where_params, 'D');
                bufferdata_push(&where_params, ' ');
            }

            for (size_t k = 0; k < strlen(field->name); k++)
                bufferdata_push(&where_params, field->name[k]);

            bufferdata_push(&where_params, '=');
            bufferdata_push(&where_params, '\'');

            for (size_t j = 0; j < v->_length; j++)
                bufferdata_push(&where_params, fieldvalue[j]);

            bufferdata_push(&where_params, '\'');

            iter_where++;

            break;
        }
    }

    bufferdata_complete(&where_params);

    dbresult_t result = dbquery(&dbinst,
        "SET ROLE slave_delete;"
        "DELETE FROM "
            "%s "
        "WHERE "
            "%s "
        ,
        model->table(model),
        bufferdata_get(&where_params)
    );

    if (!dbresult_ok(&result))
        goto failed;

    res = 1;

    failed:

    bufferdata_clear(&where_params);
    dbresult_free(&result);

    return res;
}

int model_int(mfield_int_t* field) {
    if (field == NULL) return 0;
    if (field->type != MODEL_INT) return 0;

    return field->value._int;
}

const char* model_string(mfield_string_t* field) {
    if (field == NULL) return NULL;
    if (field->type != MODEL_STRING) return NULL;

    return field->value._string;
}

void model_set_int(mfield_int_t* field, int value) {
    if (field == NULL) return;
    if (field->type != MODEL_INT) return;

    field->value._int = value;
}

void model_set_string(mfield_string_t* field, const char* value) {
    model_set_stringn(field, value, strlen(value));
}

void model_set_stringn(mfield_string_t* field, const char* value, size_t size) {
    if (field == NULL) return;
    if (field->type != MODEL_STRING) return;

    if (!field->dirty) {
        field->oldvalue._string = field->value._string;
        field->oldvalue._length = field->value._length;
        field->dirty = 1;
        field->value._string = NULL;
        field->value._length = 0;
    }

    if (field->value._string != NULL)
        free(field->value._string);

    field->value._string = strdup(value);
    field->value._length = size;
}

const char* mvalue_to_string(mvalue_t* value, const mtype_e type) {
    if (value == NULL) return NULL;

    switch (type) {
    case MODEL_INT: {
        value->_string = malloc(32);
        if (value->_string == NULL)
            return NULL;

        value->_length = snprintf(value->_string, 32, "%d", value->_int);
        break;
    }
    case MODEL_STRING: {
        break;
    }
    default:
        break;
    }

    return value->_string;
}

int mstr_to_int(const char* value) {
    return atoi(value);
}

void mvalue_clear(mvalue_t* value) {
    if (value == NULL) return;

    value->_int = 0;
    value->_length = 0;

    if (value->_string != NULL)
        free(value->_string);

    value->_string = NULL;
}
