#ifndef __MODULELOADER__
#define __MODULELOADER__

#include "appconfig.h"

int module_loader_load_json_config(const char* path, jsondoc_t** document);
int module_loader_init(appconfig_t* config);
int module_loader_config_load(appconfig_t* config, jsondoc_t* document);
int module_loader_config_correct(const char* path);
void module_loader_threads_pause(appconfig_t* config);
void module_loader_create_config_and_init(void);
void module_loader_wakeup_all_threads(void);
void module_loader_signal_lock(void);
void module_loader_signal_unlock(void);
int module_loader_signal_locked(void);

#endif