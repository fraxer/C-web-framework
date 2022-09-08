#include <cassert>
#include <fcntl.h>
#include <cstring>
#include "../../../src/helpers/helpers.h"

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

bool TEST_fileSize_lengthTextFile() {

    try {

        char* fullPath = getFullPath("/tests/helpers/readFile/text.txt");

        int fd = open(fullPath, O_RDONLY);

        free(fullPath);

        if (fd == -1) throw "File not found";

        char* data = readFile(fd);

        return strlen(data) == 3;

    } catch(const char* message) {
        return false;
    }

    return false;
}

bool TEST_fileSize_lengthEmptyFile() {

    try {

        char* fullPath = getFullPath("/tests/helpers/readFile/empty.txt");

        int fd = open(fullPath, O_RDONLY);

        free(fullPath);

        if (fd == -1) throw "File not found";

        char* data = readFile(fd);

        return strlen(data) == 0;

    } catch(const char* message) {
        return false;
    }

    return false;
}

bool TEST_fileSize_contentTextFile() {

    try {

        char* fullPath = getFullPath("/tests/helpers/readFile/text.txt");

        int fd = open(fullPath, O_RDONLY);

        free(fullPath);

        if (fd == -1) throw "File not found";

        char* data = readFile(fd);

        return strcmp(data, "123") == 0;

    } catch(const char* message) {
        return false;
    }

    return false;
}

int main() {

    assert(TEST_fileSize_lengthTextFile());
    assert(TEST_fileSize_lengthEmptyFile());
    assert(TEST_fileSize_contentTextFile());

    return 0;
}