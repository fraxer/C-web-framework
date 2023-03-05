#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "dbresult.h"

db_table_cell_t* __dbresult_field(dbresult_t* result, const char* field);


dbresultquery_t* dbresult_query_create(int rows, int cols) {
    dbresultquery_t* query = (dbresultquery_t*)malloc(sizeof(dbresultquery_t));

    if (query == NULL) return NULL;

    query->rows = rows;
    query->cols = cols;
    query->current_row = 0;
    query->current_col = 0;
    query->fields = (db_table_cell_t**)malloc(sizeof(db_table_cell_t*) * cols);
    query->table = (db_table_cell_t**)malloc(sizeof(db_table_cell_t*) * rows * cols);
    query->next = NULL;

    if (query->fields == NULL || query->table == NULL) {
        if (query->fields == NULL) free(query->fields);
        if (query->table == NULL) free(query->table);
        if (query == NULL) free(query);

        query = NULL;
    }

    return query;
}

db_table_cell_t* dbresult_cell_create(const char* string, size_t length) {
    db_table_cell_t* cell = (db_table_cell_t*)malloc(sizeof(db_table_cell_t));

    if (cell == NULL) return NULL;

    cell->length = length;
    cell->value = (char*)malloc(length + 1);

    if (cell->value == NULL) {
        free(cell);
        return NULL;
    }

    memcpy(cell->value, string, length);

    cell->value[length] = 0;

    return cell;
}

void dbresult_query_table_insert(dbresultquery_t* query, db_table_cell_t* cell, int row, int col) {
    query->table[row * query->cols + col] = cell;
}

void dbresult_query_field_insert(dbresultquery_t* query, const char* string, int col) {
    size_t length = strlen(string);

    db_table_cell_t* cell = dbresult_cell_create(string, length);

    if (cell == NULL) return;

    query->fields[col] = cell;
}

int dbresult_ok(dbresult_t* result) {
    if (result == NULL) return 0;

    return result->ok;
}

const char* dbresult_error_message(dbresult_t* result) {
    if (result == NULL) return "Db result empty";

    return result->error_message;
}

int dbresult_error_code(dbresult_t* result) {
    if (result == NULL) return 0;

    return result->error_code;
}

void dbresult_free(dbresult_t* result) {
    dbresultquery_t* query = result->query;

    while (query) {
        dbresultquery_t* next = query->next;
        db_table_cell_t** fields = query->fields;
        db_table_cell_t** table = query->table;

        if (fields != NULL) {
            for (int i = 0; i < query->cols; i++) {
                db_table_cell_t* cell = fields[i];

                // db_cell_free(cell);
                if (cell != NULL) {
                    if (cell->value != NULL) {
                        free(cell->value);
                    }
                    free(cell);
                }
            }

            free(fields);
        }

        if (table != NULL) {
            for (int i = 0; i < query->rows; i++) {
                for (int j = 0; j < query->cols; j++) {
                    db_table_cell_t* cell = table[i * query->cols + j];

                    // db_cell_free(cell);
                    if (cell != NULL) {
                        if (cell->value != NULL) {
                            free(cell->value);
                        }
                        free(cell);
                    }
                }
            }

            free(table);
        }

        query = next;
    }

    result->query = NULL;
    result->current = NULL;
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
        db_table_cell_t* cell = query->fields[i];

        if (cell == NULL) continue;

        for (int k = 0, j = 0; k < cell->length && j < field_length; k++, j++) {
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

    db_table_cell_t* cell = query->table[row * query->cols + col];

    if (cell != NULL) return cell;

    return NULL;
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
