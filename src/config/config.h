#include <fcntl.h>
#include <unistd.h>

char* getPathConfigFile(int argc, char* argv[]);

void loadConfig(const char* path);