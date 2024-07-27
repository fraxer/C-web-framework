#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <linux/limits.h>

#include "storage.h"
#include "appconfig.h"

#define STORAGE_BUILD_PATH(path) \
    va_list args; \
    va_start(args, path_format); \
    vsnprintf(path, sizeof(path), path_format, args); \
    va_end(args); \

storage_t* __storage_find(const char* storage_name);

file_t storage_file_get(const char* storage_name, const char* path_format, ...) {
    storage_t* storage = __storage_find(storage_name);
    if (storage == NULL)
        return file_alloc();

    char path[PATH_MAX];
    STORAGE_BUILD_PATH(path);

    return storage->file_get(storage, path);
}

int storage_file_put(const char* storage_name, file_t* file, const char* path_format, ...) {
    storage_t* storage = __storage_find(storage_name);
    if (storage == NULL)
        return 0;

    char path[PATH_MAX];
    STORAGE_BUILD_PATH(path);

    return storage->file_put(storage, file, path);
}

int storage_file_content_put(const char* storage_name, file_content_t* file_content, const char* path_format, ...) {
    storage_t* storage = __storage_find(storage_name);
    if (storage == NULL)
        return 0;

    char path[PATH_MAX];
    STORAGE_BUILD_PATH(path);

    return storage->file_content_put(storage, file_content, path);
}

int storage_file_remove(const char* storage_name, const char* path_format, ...) {
    storage_t* storage = __storage_find(storage_name);
    if (storage == NULL)
        return 0;

    char path[PATH_MAX];
    STORAGE_BUILD_PATH(path);

    return storage->file_remove(storage, path);
}

int storage_file_exist(const char* storage_name, const char* path_format, ...) {
    storage_t* storage = __storage_find(storage_name);
    if (storage == NULL)
        return 0;

    char path[PATH_MAX];
    STORAGE_BUILD_PATH(path);

    return storage->file_exist(storage, path);
}

int storage_file_duplicate(const char* from_storage_name, const char* to_storage_name, const char* path_format, ...) {
    if (from_storage_name == NULL)
        return 0;
    if (to_storage_name == NULL)
        return 0;

    char path[PATH_MAX];
    STORAGE_BUILD_PATH(path);

    file_t file = storage_file_get(from_storage_name, path);
    if (!file.ok)
        return 0;

    const int result = storage_file_put(to_storage_name, &file, path);

    file.close(&file);

    return result;
}

void storages_free(storage_t* storage) {
    while (storage != NULL) {
        storage_t* next = storage->next;
        storage->free(storage);
        storage = next;
    }
}

void storage_merge_slash(char* path) {
    char* f = path;
    char* s = path;
    int find_slash = 0;

    while (*s) {
        if (find_slash && *s == '/') {
            s++;
            continue;
        }
        else
            find_slash = 0;

        if (*s == '/')
            find_slash = 1;

        *f = *s;

        f++;
        s++;
    }

    *f = 0;
}

storage_t* __storage_find(const char* storage_name) {
    storage_t* item = appconfig()->storages;
    while (item) {
        if (strcmp(item->name, storage_name) == 0)
            return item;

        item = item->next;
    }

    return NULL;
}
