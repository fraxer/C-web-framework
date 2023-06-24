#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <linux/limits.h>

#include "helpers.h"

char* helpers_read_file(int fd) {

    long int size = helpers_file_size(fd);

    char* buf = (char*)malloc(size + 1);

    if (buf == NULL) return NULL;

    long int bytesRead = read(fd, buf, size);

    if (bytesRead == -1) return NULL;

    buf[size] = 0;

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

int helpers_mkdir(const char* base_path, const char* path) {
    if (strlen(path) == 0)
        return 0;

    char local_path[PATH_MAX] = {0};
    size_t base_path_len = strlen(base_path);

    strcpy(local_path, base_path);
    if (base_path[base_path_len - 1] != '/' && path[0] != '/')
        strcat(local_path, "/");

    const char* p_path = path;
    char* p_local_path = &local_path[strlen(local_path)];

    if (*p_path == '/') {
        *p_local_path++ = *p_path;
        p_path++;
    }

    while (*p_path) {
        *p_local_path++ = *p_path;

        if (*p_path == '/') {
            p_path++;
            break;
        }

        p_path++;
    }

    *p_local_path = 0;

    struct stat stat_obj;
    if(stat(local_path, &stat_obj) == -1)
        if(mkdir(local_path, S_IRWXU) == -1) return 1;

    if (*p_path != 0)
        if (!helpers_mkdir(local_path, p_path)) return 0;

    return 1;
}