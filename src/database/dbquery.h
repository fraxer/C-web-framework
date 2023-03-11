#ifndef __DBQUERY__
#define __DBQUERY__

#include "database.h"

dbinstance_t dbinstance(db_t*, const char*);

dbresult_t dbquery(dbinstance_t*, const char*);

dbresult_t dbbegin(dbinstance_t*, transaction_level_e);

dbresult_t dbcommit(dbinstance_t*);

dbresult_t dbrollback(dbinstance_t*);

#endif