#define _GNU_SOURCE
#include <sys/stat.h>
#include <dirent.h>

#include "storagefs.h"
#include "log.h"

void __storagefs_free(void* storage);

file_t __storagefs_file_get(void* storage, const char* path);
int __storagefs_file_put(void* storage, const file_t* file, const char* path);
int __storagefs_file_content_put(void* storage, const file_content_t* file_content, const char* path);
int __storagefs_file_remove(void* storage, const char* path);
int __storagefs_file_exist(void* storage, const char* path);
int __storagefs_prepare_fullpath(storagefs_t* storage, const char* relpath, char* fullpath);
int __storagefs_create_fullpath(const char* path);
int __storagefs_dir_empty(const char* path);
int __storagefs_remove_empty_dirs(const char* basepath, const char* path);

storagefs_t* storage_create_fs(const char* storage_name, const char* root) {
    storagefs_t* storage = malloc(sizeof * storage);
    if (storage == NULL)
        return NULL;

    storage->base.type = STORAGE_TYPE_FS;
    strcpy(storage->base.name, storage_name);
    strcpy(storage->root, root);

    storage->base.free = __storagefs_free;
    storage->base.file_get = __storagefs_file_get;
    storage->base.file_put = __storagefs_file_put;
    storage->base.file_content_put = __storagefs_file_content_put;
    storage->base.file_remove = __storagefs_file_remove;
    storage->base.file_exist = __storagefs_file_exist;

    return storage;
}

void __storagefs_free(void* storage) {
    free(storage);
}

file_t __storagefs_file_get(void* storage, const char* path) {
    if (storage == NULL) return file_alloc();
    if (path == NULL) return file_alloc();
    if (path[0] == 0) {
        log_error("Storage fs empty path\n");
        return file_alloc();
    }

    storagefs_t* s = storage;
    char fullpath[PATH_MAX];

    if (!__storagefs_prepare_fullpath(s, path, fullpath))
        return file_alloc();

    return file_open(fullpath, O_RDONLY);
}

int __storagefs_file_put(void* storage, const file_t* file, const char* path) {
    if (storage == NULL) return 0;
    if (file == NULL) return 0;
    if (path == NULL) return 0;

    file_content_t file_content = file_content_create(file->fd, file->name, 0, file->size);

    return __storagefs_file_content_put(storage, &file_content, path);
}

int __storagefs_file_content_put(void* storage, const file_content_t* file_content, const char* path) {
    if (path[0] == 0) {
        log_error("Storage fs empty path\n");
        return 0;
    }
    if (!file_content->ok) return 0;

    storagefs_t* s = storage;
    char fullpath[PATH_MAX];

    if (!__storagefs_prepare_fullpath(s, path, fullpath))
        return 0;
    if (!__storagefs_create_fullpath(fullpath))
        return 0;

    file_t target_file = file_open(fullpath, O_CREAT | O_RDWR);
    if (!target_file.ok) return 0;

    lseek(file_content->fd, 0, SEEK_SET);
    off_t offset = file_content->offset;
    if (sendfile(target_file.fd, file_content->fd, &offset, file_content->size) == -1) {
        log_error("Storage %s write file %s error: %s\n", s->base.name, file_content->filename, strerror(errno));
        target_file.close(&target_file);
        unlink(fullpath);
        return 0;
    }
    lseek(file_content->fd, 0, SEEK_SET);

    target_file.close(&target_file);

    return 1;
}

int __storagefs_file_remove(void* storage, const char* path) {
    if (storage == NULL) return 0;
    if (path == NULL) return 0;
    if (path[0] == 0) {
        log_error("Storage fs empty path\n");
        return 0;
    }

    storagefs_t* s = storage;
    char fullpath[PATH_MAX];

    if (!__storagefs_prepare_fullpath(s, path, fullpath))
        return 0;

    int result = 1;
    if (unlink(fullpath) == -1) {
        log_error("Storage %s remove path %s error: %s\n", s->base.name, path, strerror(errno));
        result = 0;
    }

    char _fullpath[PATH_MAX];
    strcpy(_fullpath, fullpath);
    __storagefs_remove_empty_dirs(s->root, dirname(_fullpath));

    return result;
}

int __storagefs_file_exist(void* storage, const char* path) {
    if (storage == NULL) return 0;
    if (path == NULL) return 0;
    if (path[0] == 0) {
        log_error("Storage fs empty path\n");
        return 0;
    }

    storagefs_t* s = storage;
    char fullpath[PATH_MAX];

    if (!__storagefs_prepare_fullpath(s, path, fullpath))
        return 0;

    struct stat buffer;   
    return stat(fullpath, &buffer) == 0;
}

int __storagefs_prepare_fullpath(storagefs_t* storage, const char* relpath, char* fullpath) {
    if (cmpsubstr_lower(relpath, "/../")) {
        log_error("Storage %s restrict /../ in path %s\n", storage->base.name, relpath);
        return 0;
    }
    if (cmpsubstr_lower(relpath, "/..")) {
        log_error("Storage %s restrict /.. in path %s\n", storage->base.name, relpath);
        return 0;
    }
    if (cmpsubstr_lower(relpath, "../")) {
        log_error("Storage %s restrict ../ in path %s\n", storage->base.name, relpath);
        return 0;
    }

    snprintf(fullpath, PATH_MAX, "%s/%s", storage->root, relpath);
    storage_merge_slash(fullpath);

    return 1;
}

int __storagefs_create_fullpath(const char* path) {
    char fullpath[PATH_MAX];
    strcpy(fullpath, path);

    const char* dirpath = dirname(fullpath);
    struct stat st;
    if (!(stat(dirpath, &st) == 0 && S_ISDIR(st.st_mode)))
        if (!helpers_mkdir(dirpath))
            return 0;

    return 1;
}

int __storagefs_dir_empty(const char* path) {
    if (path == NULL) return -1;
    if (path[0] == 0) return -1;

    DIR* dir = opendir(path);
    if (dir == NULL) return -1;

    int empty = 1;
    struct dirent* de = NULL;
    while ((de = readdir(dir)) != NULL) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
            continue;

        empty = 0;
        break;
    }

    closedir(dir);

    return empty;
}

int __storagefs_remove_empty_dirs(const char* basepath, const char* path) {
    if (path == NULL) return 0;
    if (path[0] == 0) return 0;
    if (strcmp(basepath, path) == 0) return 1;
    if (!__storagefs_dir_empty(path)) return 1;

    remove(path);

    char _fullpath[PATH_MAX];
    strcpy(_fullpath, path);

    return __storagefs_remove_empty_dirs(basepath, dirname(_fullpath));
}
