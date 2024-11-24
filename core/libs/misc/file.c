#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <errno.h>

#include "file.h"
#include "log.h"
#include "appconfig.h"

size_t __file_size(const int fd);
int __file_set_name(file_t* file, const char* name);
char* __file_content(file_t* file);
int __file_set_content(file_t* file, const char* data, const size_t size);
int __file_append_content(file_t* file, const char* data, const size_t size);
int __file_close(file_t* file);
int __file_truncate(file_t* file, const off_t offset);
void __file_reset(file_t* file);
int __file_content_set_name(file_content_t* file_content, const char* name);
file_t __file_content_make_file(file_content_t* file_content, const char* path, const char* name);
file_t __file_content_make_tmpfile(file_content_t* file_content);
char* __file_content_content(file_content_t* file_content);
int __file_internal_set_name(char* dest, const char* src);
char* __file_internal_content(const int fd, const off_t offset, const size_t size);

file_t file_create_tmp(const char* filename) {
    file_t file = file_alloc();
    file.set_name(&file, filename);
    file.ok = 1;
    file.tmp = 1;

    char* path = create_tmppath(env()->main.tmp);
    if (path == NULL) {
        file.ok = 0;
        return file;
    }

    file.fd = mkstemp(path);
    free(path);

    if (file.fd == -1)
        file.ok = 0;

    return file;
}

file_t file_open(const char* path, const int flags) {
    file_t file = file_alloc();
    if (path == NULL) return file;
    if (path[0] == 0) return file;

    const char* filename = basename((char*)path);
    if (strcmp(filename, "/") == 0) return file;
    if (strcmp(filename, ".") == 0) return file;
    if (strcmp(filename, "..") == 0) return file;

    file.fd = open(path, flags, S_IRWXU);
    if (file.fd < 0) return file;

    file.ok = 1;
    file.size = __file_size(file.fd);
    file.set_name(&file, filename);

    return file;
}

file_content_t file_content_create(const int fd, const char* filename, const off_t offset, const size_t size) {
    file_content_t file_content = {
        .fd = fd,
        .ok = 1,
        .offset = offset,
        .size = size,
        .filename = {0},

        .set_filename = __file_content_set_name,
        .make_file = __file_content_make_file,
        .make_tmpfile = __file_content_make_tmpfile,
        .content = __file_content_content
    };
    file_content.set_filename(&file_content, filename);

    return file_content;
}

file_t file_alloc() {
    return (file_t){
        .fd = 0,
        .ok = 0,
        .size = 0,
        .name = {0},

        .set_name = __file_set_name,
        .content = __file_content,
        .set_content = __file_set_content,
        .append_content = __file_append_content,
        .close = __file_close,
        .truncate = __file_truncate,
    };
}

size_t __file_size(const int fd) {
    struct stat stat_buf;
    int r = fstat(fd, &stat_buf);

    return r == 0 ? stat_buf.st_size : -1;
}

int __file_set_name(file_t* file, const char* name) {
    return __file_internal_set_name(file->name, name);
}

char* __file_content(file_t* file) {
    if (file == NULL) return NULL;

    const off_t offset = 0;
    return __file_internal_content(file->fd, offset, file->size);
}

int __file_set_content(file_t* file, const char* data, const size_t size) {
    if (file == NULL) return 0;
    if (file->fd <= 0) return 0;

    lseek(file->fd, 0, SEEK_SET);
    const int r = write(file->fd, data, size);
    lseek(file->fd, 0, SEEK_SET);

    if (r == -1)
        return 0;

    file->size = r;

    return r;
}

int __file_append_content(file_t* file, const char* data, const size_t size) {
    if (file == NULL) return 0;
    if (file->fd <= 0) return 0;

    lseek(file->fd, file->size, SEEK_SET);
    const int r = write(file->fd, data, size);
    lseek(file->fd, 0, SEEK_SET);

    file->size += r;

    return r > 0;
}

int __file_close(file_t* file) {
    if (file->fd == 0) return 1;

    char path[PATH_MAX];
    snprintf(path, sizeof(path), "/proc/self/fd/%d", file->fd);

    if (file->tmp) {
        char filePath[PATH_MAX];
        const int rlresult = readlink(path, filePath, PATH_MAX);
        if (rlresult == -1)
            log_error("File: readlink error");
        if (rlresult >= 0)
            unlink(filePath);
    }

    const int status = close(file->fd);

    __file_reset(file);

    return status == 0;
}

int __file_truncate(file_t* file, const off_t offset) {
    if (file->fd == 0) return 0;

    const int status = ftruncate(file->fd, offset);
    if (status == 0) {
        file->size = 0;
        return 1;
    }

    return 0;
}

void __file_reset(file_t* file) {
    file->fd = 0;
    file->ok = 0;
    file->size = 0;
    memset(file->name, 0, NAME_MAX);
}

int __file_content_set_name(file_content_t* file_content, const char* name) {
    return __file_internal_set_name(file_content->filename, name);
}

file_t __file_content_make_file(file_content_t* file_content, const char* path, const char* name) {
    if (file_content == NULL) return file_alloc();
    if (name == NULL)
        name = file_content->filename;

    const char* pname = name;
    if (pname[0] == '/')
        pname++;

    char fullpath[PATH_MAX] = {0};
    strcpy(fullpath, path);
    if (fullpath[strlen(fullpath) - 1] != '/')
        strcat(fullpath, "/");

    strcat(fullpath, pname);
    file_t file = file_open(fullpath, O_CREAT | O_RDWR);
    if (!file.ok) return file;

    lseek(file_content->fd, 0, SEEK_SET);
    off_t offset = file_content->offset;
    if (sendfile(file.fd, file_content->fd, &offset, file_content->size) == -1) {
        log_error("File error: %s\n", strerror(errno));
        file.close(&file);
        unlink(fullpath);
        return file;
    }

    lseek(file_content->fd, 0, SEEK_SET);
    lseek(file.fd, 0, SEEK_SET);

    file.ok = 1;
    file.size = file_content->size;

    return file;
}

file_t __file_content_make_tmpfile(file_content_t* file_content) {
    if (file_content == NULL) return file_alloc();

    file_t file = file_create_tmp(file_content->filename);

    lseek(file_content->fd, 0, SEEK_SET);
    off_t offset = file_content->offset;
    if (sendfile(file.fd, file_content->fd, &offset, file_content->size) == -1) {
        log_error("Tmpfile error: %s\n", strerror(errno));
        file.close(&file);
        return file;
    }

    lseek(file_content->fd, 0, SEEK_SET);
    lseek(file.fd, 0, SEEK_SET);

    file.ok = 1;
    file.size = file_content->size;

    return file;
}

char* __file_content_content(file_content_t* file_content) {
    if (file_content == NULL) return NULL;

    return __file_internal_content(file_content->fd, file_content->offset, file_content->size);
}

int __file_internal_set_name(char* dest, const char* src) {
    if (src == NULL) return 0;
    if (src[0] == 0) return 0;

    const char* psrc = src;
    if (psrc[0] == '/')
        psrc++;

    const char* filename = basename((char*)psrc);
    if (strcmp(filename, "/") == 0) return 0;
    if (strcmp(filename, ".") == 0) return 0;
    if (strcmp(filename, "..") == 0) return 0;

    size_t length = strlen(filename);
    if (length >= NAME_MAX)
        length = NAME_MAX - 1;

    strncpy(dest, filename, length);
    dest[length] = 0;

    return 1;
}

char* __file_internal_content(const int fd, const off_t offset, const size_t size) {
    if (fd <= 0) return NULL;
    if (size <= 0) return NULL;

    char* buffer = malloc(size + 1);
    if (buffer == NULL) return NULL;

    lseek(fd, offset, SEEK_SET);
    const int r = read(fd, buffer, size);
    lseek(fd, 0, SEEK_SET);

    if (r <= 0) {
        free(buffer);
        return NULL;
    }

    buffer[size] = 0;

    return buffer;
}