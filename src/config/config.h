#ifndef __CONFIG__
#define __CONFIG__

#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include "../helpers/helpers.h"

char* getConfigPath(int argc, char* argv[]);

void saveConfigPath(const char* path);

void loadConfig(const char* path);

void reloadConfig();

#endif