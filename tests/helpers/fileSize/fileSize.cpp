#include <cassert>
#include <fcntl.h>
#include <cstring>
#include "../../../tests/common.h"
#include "../../../src/helpers/helpers.h"

bool TEST_fileSize_textFile() {

    char* fullPath = getFullPath("/tests/helpers/fileSize/text.txt");

    int fd = open(fullPath, O_RDONLY);

    free(fullPath);

    if (fd == -1) return false;

    return fileSize(fd) == 3;
}

bool TEST_fileSize_emptyFile() {

    char* fullPath = getFullPath("/tests/helpers/fileSize/empty.txt");

    int fd = open(fullPath, O_RDONLY);

    free(fullPath);

    if (fd == -1) return false;

    return fileSize(fd) == 0;
}

bool TEST_fileSize_failed() {
    return fileSize(-1) == -1;
}

int main() {

    assert(TEST_fileSize_textFile());
    assert(TEST_fileSize_emptyFile());
    assert(TEST_fileSize_failed());

    return 0;
}