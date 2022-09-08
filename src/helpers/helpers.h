#ifndef __HELPERS__
#define __HELPERS__

#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>

char* readFile(int fd);

long int fileSize(int fd);

#endif