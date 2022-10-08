#include <cassert>
#include <fcntl.h>
#include <cstring>
#include "../../../tests/common.h"
#include "../../../src/helpers/helpers.h"

bool TEST_fileSize_lengthEmptyFile() {

    char* fullPath = getFullPath("/tests/helpers/readFile/empty.txt");

    int fd = open(fullPath, O_RDONLY);

    free(fullPath);

    if (fd == -1) return false;

    char* data = readFile(fd);

    return strlen(data) == 0;
}

bool TEST_fileSize_contentTextFile() {

    char* fullPath = getFullPath("/tests/helpers/readFile/text.txt");

    int fd = open(fullPath, O_RDONLY);

    free(fullPath);

    if (fd == -1) return false;

    char* data = readFile(fd);

    return strcmp(data, "123") == 0;
}

bool TEST_fileSize_failed() {
    return readFile(-1) == nullptr;
}

int main() {

    assert(TEST_fileSize_lengthEmptyFile());
    assert(TEST_fileSize_contentTextFile());
    assert(TEST_fileSize_failed());

    return 0;
}