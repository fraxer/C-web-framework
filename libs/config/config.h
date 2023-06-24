#ifndef __CONFIG__
#define __CONFIG__

#include "../json/json.h"

char* config_get_path(int argc, char* argv[]);

int config_save_path(const char* path);

int config_load(const char* path);

int config_reload();

int config_init(int argc, char* argv[]);

int config_free();

int config_parse_data(const char* data);

const jsontok_t* config_get_section(const char*);

#endif