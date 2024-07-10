#ifndef __STORAGE__
#define __STORAGE__

#include <stdio.h>
#include <linux/limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/sendfile.h>

#include "file.h"
#include "helpers.h"

typedef enum {
    STORAGE_TYPE_FS = 0,
    STORAGE_TYPE_S3,
} storage_type_e;

typedef struct storage {
    storage_type_e type;
    void(*free)(void* storage);
    char name[NAME_MAX];

    file_t(*file_get)(void* storage, const char* path);
    int(*file_put)(void* storage, const file_t* file, const char* path);
    int(*file_content_put)(void* storage, const file_content_t* file_content, const char* path);
    int(*file_remove)(void* storage, const char* path);
    int(*file_exist)(void* storage, const char* path);

    struct storage* next;
} storage_t;

file_t storage_file_get(const char* storage_name, const char* path_format, ...);
int storage_file_put(const char* storage_name, file_t* file, const char* path_format, ...);
int storage_file_content_put(const char* storage_name, file_content_t* file_content, const char* path_format, ...);
int storage_file_remove(const char* storage_name, const char* path_format, ...);
int storage_file_exist(const char* storage_name, const char* path_format, ...);
int storage_file_duplicate(const char* from_storage_name, const char* to_storage_name, const char* path_format, ...);
void storages_free(storage_t* storage);
void storage_merge_slash(char* path);

#endif
