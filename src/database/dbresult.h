#ifndef __DBRESULT__
#define __DBRESULT__

#include "database.h"

dbresultquery_t* dbresult_query_create(int, int);

int dbresult_cell_create(db_table_cell_t*, const char*, size_t);

void dbresult_query_table_insert(dbresultquery_t*, const char*, size_t, int, int);

void dbresult_query_field_insert(dbresultquery_t*, const char*, int);

int dbresult_ok(dbresult_t*);

const char* dbresult_error_message(dbresult_t*);

int dbresult_error_code(dbresult_t*);

void dbresult_free(dbresult_t*);

int dbresult_row_next(dbresult_t*);

int dbresult_col_next(dbresult_t*);

int dbresult_row_set(dbresult_t*, int);

int dbresult_col_set(dbresult_t*, int);

dbresultquery_t* dbresult_query_next(dbresult_t*);

int dbresult_query_rows(dbresult_t*);

int dbresult_query_cols(dbresult_t*);

db_table_cell_t* dbresult_field(dbresult_t*, const char*);

db_table_cell_t* dbresult_cell(dbresult_t*, int, int);

int dbresult_query_first(dbresult_t*);

int dbresult_row_first(dbresult_t*);

int dbresult_col_first(dbresult_t*);

#endif