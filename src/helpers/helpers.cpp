#include "helpers.h"

char* readFile(int fd) {

    long int size = fileSize(fd);

    char* buf = (char*)malloc(size + 1);

    if (buf == nullptr) return nullptr;

    long int bytesRead = read(fd, buf, size);

    if (bytesRead == -1) return nullptr;

    return buf;
}

long int fileSize(int fd) {

    struct stat stat_buf;

    int r = fstat(fd, &stat_buf);

    return r == 0 ? stat_buf.st_size : -1;
}
