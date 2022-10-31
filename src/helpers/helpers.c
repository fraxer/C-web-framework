#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include "helpers.h"

char* helpers_read_file(int fd) {

    long int size = helpers_file_size(fd);

    char* buf = (char*)malloc(size + 1);

    if (buf == NULL) return NULL;

    long int bytesRead = read(fd, buf, size);

    if (bytesRead == -1) return NULL;

    return buf;
}

long int helpers_file_size(int fd) {

    struct stat stat_buf;

    int r = fstat(fd, &stat_buf);

    return r == 0 ? stat_buf.st_size : -1;
}

void helpers_free_null(void* ptr) {
    free(ptr);

    ptr = NULL;
}
