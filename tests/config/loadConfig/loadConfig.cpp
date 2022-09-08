#include <cassert>
#include "../../../src/config/config.h"

char* getFullPath(const char* filePath) {

    char* fullPath = (char*)malloc(
        sizeof(char) * strlen(CMAKE_SOURCE_DIR) +
        sizeof(char) * strlen(filePath) +
        1
    );

    if (fullPath == nullptr) {
        throw "Error allocate memory for fullPath";
    }

    strcpy(fullPath, CMAKE_SOURCE_DIR);
    strcat(fullPath, filePath);

    return fullPath;
}

bool TEST_loadConfig_correctPath() {

    char* path = nullptr;

    try {
        path = getFullPath("/tests/config/loadConfig/config.json");

        loadConfig(path);

        free(path);
    } catch (const char* message) {
        free(path);

        return false;
    }

    return true;
}

bool TEST_loadConfig_incorrectPath() {

    char* path = nullptr;

    try {
        path = getFullPath("/tests/config/loadConfig/undefined.txt");

        loadConfig(path);

        free(path);

        return true;
    } catch (const char* message) {
        free(path);

        return false;
    }

    return false;
}

int main() {

    assert( TEST_loadConfig_correctPath());
    assert(!TEST_loadConfig_incorrectPath());

    return 0;
}