#ifndef __MODULELOADER__
#define __MODULELOADER__

#include "../database/database.h"
#include "../json/json.h"

int module_loader_init(int, char*[]);

int module_loader_reload();

db_t* module_loader_databases_load(const jsontok_t*);

#endif