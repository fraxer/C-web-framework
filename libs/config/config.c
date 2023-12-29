#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>

#include "helpers.h"
#include "log.h"
#include "config.h"

static char* _config_path = NULL;
static jsondoc_t* _document = NULL;
static config_t* _config = NULL;

int __config_load(const char* path);
int __config_parse_data(const char* data);
const char* __config_get_path(int argc, char* argv[]);
int __config_save_path(const char* path);
int __config_fill_struct();

const char* __config_get_path(int argc, char* argv[]) {
    int opt = 0;
    const char* path = NULL;
    int c_found = 0;

    opterr = 0;
    optopt = 0;
    optind = 0;
    optarg = NULL;

    while ((opt = getopt(argc, argv, "c:")) != -1) {
        switch (opt) {
            case 'c':
                path = optarg;
                c_found = 1;
                break;
            default:
                return NULL;
        }
    }

    if (!c_found)
        return NULL;
    if (argc < 3)
        return NULL;

    return path;
}

int __config_save_path(const char* path) {
    if (path == NULL)
        return 0;

    if (strlen(path) == 0)
        return 0;

    if (_config_path != NULL)
        free(_config_path);

    _config_path = malloc(sizeof(char) * strlen(path) + 1);

    if (_config_path == NULL)
        return 0;

    strcpy(_config_path, path);

    return 1;
}

int __config_load(const char* path) {
    if (path == NULL) return 0;

    int fd = open(path, O_RDONLY);
    if (fd == -1) return 0;

    char* data = helpers_read_file(fd);

    close(fd);

    if (data == NULL) return 0;

    int result = 0;

    if (!__config_parse_data(data)) goto failed;

    result = 1;

    failed:

    free(data);

    return result;
}

int config_reload() {
    if (_config_path == NULL)
        return 0;

    if (strlen(_config_path) == 0)
        return 0;

    config_free();

    return __config_load(_config_path);
}

int config_init(int argc, char* argv[]) {
    _config = malloc(sizeof * _config);
    if (_config == NULL)
        return 0;

    const char* config_path = __config_get_path(argc, argv);
    if (config_path == NULL) {
        log_print("Usage: -c <path to config file>\n", "");
        return 0;
    }

    if (__config_load(config_path) == -1) {
        log_print("Can't load config\n", "");
        return 0;
    }

    if (!__config_save_path(config_path)) {
        log_print("Can't save config path\n", "");
        return 0;
    }

    return 1;
}

void config_free() {
    json_free(_document);
}

int __config_parse_data(const char* data) {
    _document = json_init();
    if (_document == NULL) return 0;

    if (!json_parse(_document, data)) return 0;

    const jsontok_t* root = json_root(_document);
    if (!json_is_object(root)) return 0;

    if (!__config_fill_struct())
        return 0;

    return 1;
}

const config_t* config() {
    return _config;
}

void config_set(const config_t* config) {
    _config = (config_t*)config;
}

int __config_fill_struct() {
    const jsontok_t* root = json_root(_document);

    const jsontok_t* token_migrations = json_object_get(root, "migrations");
    if (token_migrations == NULL) {
        _config->migrations.source_directory = "";
    }
    else {
        if (!json_is_object(token_migrations)) return 0;

        const jsontok_t* token_source_directory = json_object_get(token_migrations, "source_directory");
        if (!json_is_string(token_source_directory)) return 0;
        _config->migrations.source_directory = json_string(token_source_directory);
    }

    const jsontok_t* token_main = json_object_get(root, "main");
    if (!json_is_object(token_main)) return 0;

    const jsontok_t* token_workers = json_object_get(token_main, "workers");
    if (!json_is_int(token_workers)) return 0;
    _config->main.workers = json_uint(token_workers);

    const jsontok_t* token_threads = json_object_get(token_main, "threads");
    if (!json_is_int(token_threads)) return 0;
    _config->main.threads = json_uint(token_threads);

    const jsontok_t* token_reload = json_object_get(token_main, "reload");
    if (!json_is_string(token_reload)) return 0;
    _config->main.reload = (strcmp(json_string(token_reload), "hard") == 0)
        ? CONFIG_RELOAD_HARD
        : CONFIG_RELOAD_SOFT;

    const jsontok_t* token_read_buffer = json_object_get(token_main, "read_buffer");
    if (!json_is_int(token_read_buffer)) return 0;
    _config->main.read_buffer = json_uint(token_read_buffer);

    const jsontok_t* token_client_max_body_size = json_object_get(token_main, "client_max_body_size");
    if (!json_is_int(token_client_max_body_size)) return 0;
    _config->main.client_max_body_size = json_uint(token_client_max_body_size);

    const jsontok_t* token_tmp = json_object_get(token_main, "tmp");
    if (!json_is_string(token_tmp)) return 0;
    _config->main.tmp = json_string(token_tmp);
    const size_t tmp_length = strlen(_config->main.tmp);
    if (_config->main.tmp[tmp_length - 1] == '/') {
        log_print("[Error][Config] Remove last slash from main.tmp\n");
        return 0;
    }

    const jsontok_t* token_gzip = json_object_get(token_main, "gzip");
    if (!json_is_array(token_gzip)) return 0;
    _config->main.gzip = token_gzip;


    const jsontok_t* token_servers = json_object_get(root, "servers");
    if (!json_is_object(token_servers)) return 0;
    _config->servers = token_servers;

    const jsontok_t* token_mimetypes = json_object_get(root, "mimetypes");
    if (!json_is_object(token_mimetypes)) return 0;
    _config->mimetypes = token_mimetypes;

    return 1;
}
