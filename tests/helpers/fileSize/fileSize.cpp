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

bool TEST_fileSize_textFile() {

    try {

        char* fullPath = getFullPath("/tests/helpers/fileSize/text.txt");

        int fd = open(fullPath, O_RDONLY);

        free(fullPath);

        if (fd == -1) throw "File not found";

        return fileSize(fd) == 3;

    } catch(const char* message) {
        return false;
    }

    return false;
}

bool TEST_fileSize_emptyFile() {

    try {

        char* fullPath = getFullPath("/tests/helpers/fileSize/empty.txt");

        int fd = open(fullPath, O_RDONLY);

        free(fullPath);

        if (fd == -1) throw "File not found";

        return fileSize(fd) == 0;

    } catch(const char* message) {
        return false;
    }

    return false;
}

int main() {

    assert(TEST_fileSize_textFile());
    assert(TEST_fileSize_emptyFile());

    return 0;
}