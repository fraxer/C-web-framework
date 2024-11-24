#define _GNU_SOURCE
#include <sys/stat.h>

#include "storage.h"
#include "log.h"
#include "appconfig.h"
#include "sessionfile.h"

static const char* __folder = "sessions";

static char* __create(const char* data);
static char* __get(const char* session_id);
static int __update(const char* session_id, const char* data);
static int __destroy(const char* session_id);
static void __remove_expired(void);
static int __expired(const int fd);

session_t* sessionfile_init() {
    session_t* session = malloc(sizeof * session);
    if (session == NULL) return NULL;

    session->create = __create;
    session->get = __get;
    session->update = __update;
    session->destroy = __destroy;
    session->remove_expired = __remove_expired;

    return session;
}

char* __create(const char* data) {
    char* session_id = session_create_id();
    if (session_id == NULL) {
        log_error("sessionfile__create: alloc memory for id failed\n");
        return NULL;
    }

    if (!storage_file_data_put(appconfig()->sessionconfig.storage_name, data, strlen(data), "%s/%s", __folder, session_id))
        log_error("sessionfile__create: storage_file_data_put failed\n");

    return session_id;
}

char* __get(const char* session_id) {
    file_t file = storage_file_get(appconfig()->sessionconfig.storage_name, "%s/%s", __folder, session_id);
    if (!file.ok) return NULL;

    char* data = NULL;
    if (!__expired(file.fd))
        data = file.content(&file);

    file.close(&file);

    return data;
}

int __update(const char* session_id, const char* data) {
    file_t file = storage_file_get(appconfig()->sessionconfig.storage_name, "%s/%s", __folder, session_id);
    if (!file.ok) return 0;

    const int expired = __expired(file.fd);

    file.close(&file);

    if (expired)
        return 0;

    if (!storage_file_data_put(appconfig()->sessionconfig.storage_name, data, strlen(data), "%s/%s", __folder, session_id)) {
        log_error("sessionfile__update: storage_file_data_put failed\n");
        return 0;
    }

    return 1;
}

int __destroy(const char* session_id) {
    if (session_id == NULL) return 0;

    if (!storage_file_remove(appconfig()->sessionconfig.storage_name, "%s/%s", __folder, session_id)) {
        log_error("sessionfile__destroy: storage_file_remove failed\n");
        return 0;
    }

    return 1;
}

void __remove_expired(void) {
    array_t* files = storage_file_list(appconfig()->sessionconfig.storage_name, __folder);
    if (files == NULL) return;

    for (size_t i = 0; i < array_size(files); i++) {
        file_t file = storage_file_get(appconfig()->sessionconfig.storage_name, "%s", array_get(files, i));
        if (!file.ok) continue;

        const int expired = __expired(file.fd);

        file.close(&file);

        if (expired)
            storage_file_remove(appconfig()->sessionconfig.storage_name, "%s", array_get(files, i));
    }

    array_free(files);
}

int __expired(const int fd) {
    struct stat stat;
    if (fstat(fd, &stat) == 0)
        return stat.st_mtime <= time(NULL) - appconfig()->sessionconfig.lifetime;

    return 1;
}
