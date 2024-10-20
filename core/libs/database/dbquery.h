#ifndef __DBQUERY__
#define __DBQUERY__

#include "array.h"
#include "database.h"

dbinstance_t* dbinstance(const char* dbid);
void dbinstance_free(dbinstance_t* instance);
dbresult_t* dbqueryf(dbinstance_t*, const char*, ...);
dbresult_t* dbquery(dbinstance_t* instance, const char* format, array_t* params);
dbresult_t* dbtable_exist(dbinstance_t* instance, const char* table);
dbresult_t* dbtable_migration_create(dbinstance_t* instance, const char* table);
dbresult_t* dbbegin(dbinstance_t*, transaction_level_e);
dbresult_t* dbcommit(dbinstance_t*);
dbresult_t* dbrollback(dbinstance_t*);
dbresult_t* dbinsert(dbinstance_t* instance, const char* table, array_t* params);
dbresult_t* dbupdate(dbinstance_t* instance, const char* table, array_t* set, array_t* where);
dbresult_t* dbdelete(dbinstance_t* instance, const char* table, array_t* where);
dbresult_t* dbselect(dbinstance_t* instance, const char* table, array_t* columns, array_t* where);

#endif