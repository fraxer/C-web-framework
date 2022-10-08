#ifndef __CONFIG__
#define __CONFIG__

#include "rapidjson/document.h"

namespace config {

char* getPath(int argc, char* argv[]);

int savePath(const char* path);

int load(const char* path);

int reload();

int init(int argc, char* argv[]);

int free();

int parseData(const char* data);

const rapidjson::Value* getSection(const char*);

} // namespace

#endif