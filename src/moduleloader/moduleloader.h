#ifndef __MODULELOADER__
#define __MODULELOADER__

#include "../jsmn/jsmn.h"
#include "../route/route.h"
#include "../domain/domain.h"
#include "../server/server.h"

int module_loader_init(int, char*[]);

int module_loader_reload();

int module_loader_init_modules();

int module_loader_init_module(void* (*)(), int (*)(void*));

int module_loader_parse_log(void*);

route_t* module_loader_load_routes(const jsmntok_t*);

domain_t* module_loader_load_domains(const jsmntok_t*);

int module_loader_load_servers(void*);

int module_loader_parse_database(void*);

int module_loader_parse_openssl(void*);

int module_loader_parse_mimetype(void*);

int module_loader_load_threads(void*);

int module_loader_parse_yookassa(void*);

int module_loader_parse_s3(void*);

int module_loader_parse_mail(void*);

int module_loader_parse_encryption(void*);

int module_loader_set_string(const void* p_object, char** value, const char* key);

char* module_loader_get_string(const void* p_object, const char* key);

int module_loader_set_int(const void*, int* value, const char*);

int module_loader_set_ushort(const void* p_object, unsigned short* value, const char* key);

unsigned short module_loader_get_ushort(const void* p_object, const char* key);

int module_loader_setDatabaseDriver(const void* p_object, void* value, const char* key);

int module_loader_setDatabaseConnections(const void* p_object, void* p_value, const char* key);

#endif