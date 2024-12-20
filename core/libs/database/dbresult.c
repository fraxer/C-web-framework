#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "dbresult.h"

db_table_cell_t* __dbresult_field(dbresult_t* result, const char* field);


dbresult_t* dbresult_create(void) {
    dbresult_t* result = malloc(sizeof * result);
    if (result == NULL) return NULL;

    result->ok = 0;
    result->query = NULL;
    result->current = NULL;

    return result;
}

dbresultquery_t* dbresult_query_create(int rows, int cols) {
    dbresultquery_t* query = malloc(sizeof * query);
    if (query == NULL) return NULL;

    query->rows = rows;
    query->cols = cols;
    query->current_row = 0;
    query->current_col = 0;
    query->fields = malloc(cols * sizeof(db_table_cell_t));
    query->table = malloc(rows * cols * sizeof(db_table_cell_t));
    query->next = NULL;

    if (query->fields == NULL || query->table == NULL) {
        dbresult_query_free(query);
        return NULL;
    }

    return query;
}

void dbresult_query_free(dbresultquery_t* query) {
    if (query == NULL) return;

    if (query->fields) free(query->fields);
    if (query->table) free(query->table);
    free(query);
}

int dbresult_cell_create(db_table_cell_t* cell, const char* string, size_t length) {
    if (cell == NULL) return -1;

    cell->length = length;
    cell->value = malloc(length + 1);

    if (cell->value == NULL) return -1;

    memcpy(cell->value, string, length);

    cell->value[length] = 0;

    return 0;
}

void dbresult_query_value_insert(dbresultquery_t* query, const char* string, size_t length, int row, int col) {
    dbresult_cell_create(&query->table[row * query->cols + col], string, length);
}

void dbresult_query_field_insert(dbresultquery_t* query, const char* string, int col) {
    size_t length = strlen(string);

    dbresult_cell_create(&query->fields[col], string, length);
}

int dbresult_ok(dbresult_t* result) {
    if (result == NULL) return 0;

    return result->ok;
}

void dbresult_free(dbresult_t* result) {
    if (result == NULL) return;

    dbresultquery_t* query = result->query;
    while (query) {
        dbresultquery_t* next = query->next;

        if (query->fields)
            for (int i = 0; i < query->cols; i++)
                db_cell_free(&query->fields[i]);

        if (query->table)
            for (int i = 0; i < query->rows * query->cols; i++)
                db_cell_free(&query->table[i]);

        dbresult_query_free(query);

        query = next;
    }

    free(result);
}

int dbresult_row_next(dbresult_t* result) {
    dbresultquery_t* query = result->current;

    if (query == NULL) return 0;
    if (query->current_row + 1 == query->rows) return 0;

    query->current_row++;

    return 1;
}

int dbresult_col_next(dbresult_t* result) {
    dbresultquery_t* query = result->current;

    if (query == NULL) return 0;
    if (query->current_col + 1 == query->cols) return 0;

    query->current_col++;

    return 1;
}

int dbresult_row_set(dbresult_t* result, int index) {
    dbresultquery_t* query = result->current;

    if (query == NULL) return 0;
    if (index + 1 == query->rows) return 0;

    query->current_row = index;

    return 1;
}

int dbresult_col_set(dbresult_t* result, int index) {
    dbresultquery_t* query = result->current;

    if (query == NULL) return 0;
    if (index + 1 == query->cols) return 0;

    query->current_col = index;

    return 1;
}

dbresultquery_t* dbresult_query_next(dbresult_t* result) {
    if (result->current == NULL) return NULL;
    if (result->current->next == NULL) return NULL;

    result->current = result->current->next;

    return result->current;
}

int dbresult_query_rows(dbresult_t* result) {
    if (result->current == NULL) return 0;

    return result->current->rows;
}

int dbresult_query_cols(dbresult_t* result) {
    if (result->current == NULL) return 0;

    return result->current->cols;
}

db_table_cell_t* dbresult_field(dbresult_t* result, const char* field) {
    if (field == NULL) return dbresult_cell(result, result->current->current_row, result->current->current_col);

    return __dbresult_field(result, field);
}

db_table_cell_t* __dbresult_field(dbresult_t* result, const char* field) {
    dbresultquery_t* query = result->current;

    if (query == NULL) return NULL;
    if (query->fields == NULL) return NULL;
    if (query->table == NULL) return NULL;

    size_t field_length = strlen(field);
    int current_col = -1;

    for (int i = 0; i < query->cols; i++) {
        db_table_cell_t* cell = &query->fields[i];

        if (cell == NULL) continue;

        for (size_t k = 0, j = 0; k < cell->length && j < field_length; k++, j++) {
            if (cell->value[k] != field[j]) break;
            current_col = i;
        }

        if (current_col > -1) break;
    }

    if (current_col == -1) return NULL;

    return dbresult_cell(result, query->current_row, current_col);
}

db_table_cell_t* dbresult_cell(dbresult_t* result, int row, int col) {
    dbresultquery_t* query = result->current;

    if (query == NULL) return NULL;
    if (query->table == NULL) return NULL;
    if (row >= query->rows || col >= query->cols) return NULL;

    return &query->table[row * query->cols + col];
}

int dbresult_query_first(dbresult_t* result) {
    if (result == NULL) return 0;
    if (result->query == NULL) return 0;
    if (result->current == NULL) return 0;

    result->current = result->query;

    return 1;
}

int dbresult_row_first(dbresult_t* result) {
    if (result == NULL) return 0;
    if (result->current == NULL) return 0;

    result->current->current_row = 0;

    return 1;
}

int dbresult_col_first(dbresult_t* result) {
    if (result == NULL) return 0;
    if (result->current == NULL) return 0;

    result->current->current_col = 0;

    return 1;
}
