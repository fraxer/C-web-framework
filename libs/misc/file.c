#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <errno.h>
#include <linux/limits.h>

#include "file.h"
#include "log.h"
#include "helpers.h"

static const size_t max_filename = 255;

size_t __file_size(const int fd);
const char* __file_name(file_t* file);
int __file_set_name(file_t* file, const char* name);
char* __file_content(file_t* file);
int __file_set_content(file_t* file, const char* data, const size_t size);
int __file_append_content(file_t* file, const char* data, const size_t size);
int __file_close(file_t* file);
int __file_truncate(file_t* file, const off_t offset);
void __file_reset(file_t* file);
int __file_content_set_name(file_content_t* file_content, const char* name);
file_t __file_content_make_file(file_content_t* file_content, const char* path, const char* name);
char* __file_content_content(file_content_t* file_content);
int __file_internal_set_name(char* dest, const char* src);
char* __file_internal_content(const int fd, const off_t offset, const size_t size);

file_t file_create(const char* filename) {
    file_t file = file_alloc();
    file.set_name(&file, filename);
    file.ok = 1;

    return file;
}

file_t file_open(const char* path) {
    file_t file = file_alloc();
    if (path == NULL) return file;
    if (path[0] == 0) return file;

    {
        char fullpath[PATH_MAX] = {0};
        strcpy(fullpath, path);

        const char* dirpath = dirname(fullpath);
        struct stat st;
        if (!(stat(dirpath, &st) == 0 && S_ISDIR(st.st_mode)))
            if (!helpers_mkdir(dirpath))
                return file;
    }

    const char* filename = basename((char*)path);
    if (strcmp(filename, "/") == 0) return file;
    if (strcmp(filename, ".") == 0) return file;
    if (strcmp(filename, "..") == 0) return file;

    file.fd = open(path, O_CREAT | O_RDWR, S_IRWXU);
    if (file.fd < 0) return file;

    file.ok = 1;
    file.size = __file_size(file.fd);
    file.set_name(&file, basename((char*)path));

    return file;
}

file_content_t file_content_create(const int fd, const char* filename, const off_t offset, const size_t size) {
    file_content_t file_content = {
        .fd = fd,
        .ok = 1,
        .offset = offset,
        .size = size,
        ._filename = {0},

        .set_filename = __file_content_set_name,
        .make_file = __file_content_make_file,
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
        ._name = {0},

        .name = __file_name,
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

const char* __file_name(file_t* file) {
    return file->_name;
}

int __file_set_name(file_t* file, const char* name) {
    return __file_internal_set_name(file->_name, name);
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

    file->size = r;

    return r > 0;
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
    memset(file->_name, 0, max_filename);
}

int __file_content_set_name(file_content_t* file_content, const char* name) {
    return __file_internal_set_name(file_content->_filename, name);
}

file_t __file_content_make_file(file_content_t* file_content, const char* path, const char* name) {
    if (file_content == NULL) return file_alloc();
    if (name == NULL)
        name = file_content->_filename;

    const char* pname = name;
    if (pname[0] == '/')
        pname++;

    char fullpath[PATH_MAX] = {0};
    strcpy(fullpath, path);
    if (fullpath[strlen(fullpath) - 1] != '/')
        strcat(fullpath, "/");

    strcat(fullpath, pname);
    file_t file = file_open(fullpath);
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
    if (length >= max_filename)
        length = max_filename - 1;

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