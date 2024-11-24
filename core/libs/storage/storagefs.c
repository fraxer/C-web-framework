#define _GNU_SOURCE
#include <sys/stat.h>
#include <dirent.h>
#include <glob.h>

#include "array.h"
#include "log.h"
#include "storagefs.h"

static void __free(void* storage);
static file_t __file_get(void* storage, const char* path);
static int __file_put(void* storage, const file_t* file, const char* path);
static int __file_content_put(void* storage, const file_content_t* file_content, const char* path);
static int __file_data_put(void* storage, const char* data, const size_t data_size, const char* path);
static int __file_remove(void* storage, const char* path);
static int __file_exist(void* storage, const char* path);
static array_t* __file_list(void* storage, const char* path);
static int __prepare_fullpath(storagefs_t* storage, const char* relpath, char* fullpath);
static int __create_fullpath(const char* path);
static int __dir_empty(const char* path);
static int __remove_empty_dirs(const char* basepath, const char* path);
static int __pattern_in(const char* path);

storagefs_t* storage_create_fs(const char* storage_name, const char* root) {
    storagefs_t* storage = malloc(sizeof * storage);
    if (storage == NULL)
        return NULL;

    storage->base.type = STORAGE_TYPE_FS;
    storage->base.next = NULL;
    strcpy(storage->base.name, storage_name);
    strcpy(storage->root, root);

    storage->base.free = __free;
    storage->base.file_get = __file_get;
    storage->base.file_put = __file_put;
    storage->base.file_content_put = __file_content_put;
    storage->base.file_data_put = __file_data_put;
    storage->base.file_remove = __file_remove;
    storage->base.file_exist = __file_exist;
    storage->base.file_list = __file_list;

    return storage;
}

void __free(void* storage) {
    free(storage);
}

file_t __file_get(void* storage, const char* path) {
    if (storage == NULL) return file_alloc();
    if (path == NULL) return file_alloc();
    if (path[0] == 0) {
        log_error("Storage fs empty path\n");
        return file_alloc();
    }

    storagefs_t* s = storage;
    char fullpath[PATH_MAX];

    if (!__prepare_fullpath(s, path, fullpath))
        return file_alloc();

    if (__pattern_in(path)) {
        glob_t glob_result;
        glob_result.gl_pathc = 0;
        glob_result.gl_pathv = NULL;

        if (glob(fullpath, GLOB_TILDE, NULL, &glob_result) == 0) {
            if (glob_result.gl_pathc > 0)
                strcpy(fullpath, glob_result.gl_pathv[0]);

            globfree(&glob_result);
        }
    }

    return file_open(fullpath, O_RDONLY);
}

int __file_put(void* storage, const file_t* file, const char* path) {
    if (storage == NULL) return 0;
    if (file == NULL) return 0;
    if (path == NULL) return 0;

    file_content_t file_content = file_content_create(file->fd, file->name, 0, file->size);

    return __file_content_put(storage, &file_content, path);
}

int __file_content_put(void* storage, const file_content_t* file_content, const char* path) {
    if (path[0] == 0) {
        log_error("Storage fs empty path\n");
        return 0;
    }
    if (!file_content->ok) return 0;

    storagefs_t* s = storage;
    char fullpath[PATH_MAX];

    if (!__prepare_fullpath(s, path, fullpath))
        return 0;
    if (!__create_fullpath(fullpath))
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
    if (file_content->size < target_file.size)
        ftruncate(target_file.fd, file_content->size);

    lseek(file_content->fd, 0, SEEK_SET);

    target_file.close(&target_file);

    return 1;
}

int __file_data_put(void* storage, const char* data, const size_t data_size, const char* path) {
    if (path[0] == 0) {
        log_error("Storage fs empty path\n");
        return 0;
    }
    storagefs_t* s = storage;
    char fullpath[PATH_MAX];

    if (!__prepare_fullpath(s, path, fullpath))
        return 0;
    if (!__create_fullpath(fullpath))
        return 0;

    file_t target_file = file_open(fullpath, O_CREAT | O_RDWR);
    if (!target_file.ok) return 0;

    if (!target_file.set_content(&target_file, data, data_size)) {
        log_error("Storage %s write file %s error: %s\n", s->base.name, path, strerror(errno));
        target_file.close(&target_file);
        unlink(fullpath);
        return 0;
    }

    target_file.close(&target_file);

    return 1;
}

int __file_remove(void* storage, const char* path) {
    if (storage == NULL) return 0;
    if (path == NULL) return 0;
    if (path[0] == 0) {
        log_error("Storage fs empty path\n");
        return 0;
    }

    storagefs_t* s = storage;
    char fullpath[PATH_MAX];

    if (!__prepare_fullpath(s, path, fullpath))
        return 0;

    int result = 1;
    if (unlink(fullpath) == -1) {
        log_error("Storage %s remove path %s error: %s\n", s->base.name, path, strerror(errno));
        result = 0;
    }

    char _fullpath[PATH_MAX];
    strcpy(_fullpath, fullpath);
    __remove_empty_dirs(s->root, dirname(_fullpath));

    return result;
}

int __file_exist(void* storage, const char* path) {
    if (storage == NULL) return 0;
    if (path == NULL) return 0;
    if (path[0] == 0) {
        log_error("Storage fs empty path\n");
        return 0;
    }

    storagefs_t* s = storage;
    char fullpath[PATH_MAX];

    if (!__prepare_fullpath(s, path, fullpath))
        return 0;

    struct stat buffer;   
    return stat(fullpath, &buffer) == 0;
}
array_t* __file_list(void* storage, const char* path) {
    if (storage == NULL) return NULL;
    if (path == NULL) return NULL;
    if (path[0] == 0) {
        log_error("Storage fs empty path\n");
        return NULL;
    }

    storagefs_t* s = storage;
    char fullpath[PATH_MAX];

    if (!__prepare_fullpath(s, path, fullpath))
        return NULL;

    DIR* dir = opendir(fullpath);
    if (dir == NULL) {
        fprintf(stderr, "Error opening directory %s: %s\n", fullpath, strerror(errno));
        return NULL;
    }

    rewinddir(dir);

    array_t* list = array_create();
    if (list == NULL) {
        closedir(dir);
        return NULL;
    }

    struct dirent* entry = NULL;
    char rel_filename[PATH_MAX];
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        
        const int rel_filename_len = snprintf(rel_filename, sizeof(rel_filename), "%s/%s", path, entry->d_name);
        if (rel_filename_len < 1)
            continue;

        array_push_back(list, array_create_stringn(rel_filename, rel_filename_len));
    }

    closedir(dir);

    return list;
}

int __prepare_fullpath(storagefs_t* storage, const char* relpath, char* fullpath) {
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

int __create_fullpath(const char* path) {
    char fullpath[PATH_MAX];
    strcpy(fullpath, path);

    const char* dirpath = dirname(fullpath);
    struct stat st;
    if (!(stat(dirpath, &st) == 0 && S_ISDIR(st.st_mode)))
        if (!helpers_mkdir(dirpath))
            return 0;

    return 1;
}

int __dir_empty(const char* path) {
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

int __remove_empty_dirs(const char* basepath, const char* path) {
    if (path == NULL) return 0;
    if (path[0] == 0) return 0;
    if (strcmp(basepath, path) == 0) return 1;
    if (!__dir_empty(path)) return 1;

    remove(path);

    char _fullpath[PATH_MAX];
    strcpy(_fullpath, path);

    return __remove_empty_dirs(basepath, dirname(_fullpath));
}

int __pattern_in(const char* path) {
    return strchr(path, '*') != NULL;
}
