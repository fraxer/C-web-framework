#ifndef __CFILE__
#define __CFILE__

#include <sys/types.h>
#include <libgen.h>
#include <fcntl.h>
#include <linux/limits.h>

typedef struct file {
    int fd;
    int ok;
    int tmp;
    size_t size;
    char name[NAME_MAX];

    int(*set_name)(struct file*, const char* name);
    char*(*content)(struct file*);
    int(*set_content)(struct file*, const char* data, const size_t size);
    int(*append_content)(struct file*, const char* data, const size_t size);
    int(*close)(struct file*);
    int(*truncate)(struct file*, const off_t offset);
} file_t;

typedef struct file_content {
    int fd;
    int ok;
    off_t offset;
    size_t size;
    char filename[NAME_MAX];

    int(*set_filename)(struct file_content*, const char* filename);
    file_t(*make_file)(struct file_content*, const char* filepath, const char* filename);
    file_t(*make_tmpfile)(struct file_content*);
    char*(*content)(struct file_content*);
} file_content_t;

file_t file_alloc();
file_t file_create_tmp(const char* filename);
file_t file_open(const char* path, const int flags);
file_content_t file_content_create(const int fd, const char* filename, const off_t offset, const size_t size);

#endif
