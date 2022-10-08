#include <cstring>
#include <cstdlib>
#include "common.h"

char* getFullPath(const char* filePath) {

    char* fullPath = (char*)malloc(
        sizeof(char) * strlen(CMAKE_SOURCE_DIR) +
        sizeof(char) * strlen(filePath) +
        1
    );

    if (fullPath == nullptr) {
        return nullptr;
    }

    strcpy(fullPath, CMAKE_SOURCE_DIR);
    strcat(fullPath, filePath);

    return fullPath;
}
