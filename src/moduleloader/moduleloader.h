#ifndef __MODULELOADER__
#define __MODULELOADER__

#include "../jsmn/jsmn.h"
#include "../route/route.h"
#include "../domain/domain.h"
#include "../server/server.h"

int module_loader_init(int, char*[]);

int module_loader_reload();

int module_loader_init_modules();

int module_loader_reload_is_hard();

route_t* module_loader_load_routes(const jsmntok_t*);

domain_t* module_loader_load_domains(const jsmntok_t*);

int module_loader_load_servers(int);

int module_loader_load_thread_workers();

int module_loader_load_thread_handlers();

#endif