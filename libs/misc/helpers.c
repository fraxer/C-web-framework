#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <linux/limits.h>
#include <ctype.h>
#include <stdio.h>

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

int helpers_mkdir(const char* path) {
    if (path == NULL) return 0;
    if (path[0] == 0) return 0;

    return helpers_base_mkdir("/", path);
}

int helpers_base_mkdir(const char* base_path, const char* path) {
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
        if (!helpers_base_mkdir(local_path, p_path)) return 0;

    return 1;
}

int cmpstr_lower(const char* a, const char* b) {
    size_t a_length = strlen(a);
    size_t b_length = strlen(b);

    return cmpstrn_lower(a, a_length, b, b_length);
}

int cmpstrn_lower(const char *a, size_t a_length, const char *b, size_t b_length) {
    for (size_t i = 0, j = 0; i < a_length && j < b_length; i++, j++)
        if (tolower(a[i]) != tolower(b[j])) return 0;

    return 1;
}

int append_to_filefd(int fd, const char* data, size_t size) {
    if (fd <= 0) return 0;
    if (data == NULL) return 0;
    if (size == 0) return 0;

    lseek(fd, 0, SEEK_END);
    int r = write(fd, data, size);
    lseek(fd, 0, SEEK_SET);

    return r > 0;
}

char* create_tmppath(const char* tmp_path)
{
    const char* template = "tmp.XXXXXX";
    const size_t path_length = strlen(tmp_path) + strlen(template) + 2; // "/", "\0"
    char* path = malloc(path_length);
    if (path == NULL)
        return NULL;

    snprintf(path, path_length, "%s/%s", tmp_path, template);

    return path;
}

const char* file_extention(const char* path) {
    const size_t length = strlen(path);
    for (size_t i = length - 1; i > 0; i--) {
        switch (path[i]) {
        case '.':
            return &path[i + 1];
        case '/':
            return NULL;
        }
    }

    return NULL;
}

int cmpsubstr_lower(const char* a, const char* b) {
    size_t a_length = strlen(a);
    size_t b_length = strlen(b);
    size_t cmpsize = 0;

    for (size_t i = 0, j = 0; i < a_length && j < b_length; i++) {
        if (tolower(a[i]) != tolower(b[j])) {
            cmpsize = 0;
            j = 0;
            continue;
        }

        cmpsize++;
        j++;

        if (cmpsize == b_length) return 1;
    }

    return cmpsize == b_length;
}