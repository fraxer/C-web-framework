#ifndef __DBQUERY__
#define __DBQUERY__

#include "database.h"

dbinstance_t db_instance(db_t*, dbperms_e, const char*);

dbresult_t db_query(dbinstance_t*, const char*);

dbresult_t db_begin(dbinstance_t*, transaction_level_e);

dbresult_t db_commit(dbinstance_t*);

dbresult_t db_rollback(dbinstance_t*);

void db_result_free(dbresult_t*);

#endif