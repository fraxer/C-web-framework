#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>

#include "file.h"
#include "helpers.h"
#include "log.h"
#include "config.h"

static char* _config_path = NULL;
static config_t* _config = NULL;

config_t* __config_create();
int __config_load(config_t* config, const char* path);
int __config_parse_data(config_t* config, const char* data);
const char* __config_get_path(int argc, char* argv[]);
int __config_save_path(const char* path);
int __config_fill_struct(config_t* config);

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

config_t* __config_create() {
    config_t* config = malloc(sizeof * config);
    if (config == NULL)
        return NULL;

    config->main.client_max_body_size = 0;
    config->main.gzip = NULL;
    config->main.read_buffer = 0;
    config->main.reload = 0;
    config->main.threads = 0;
    config->main.tmp = NULL;
    config->main.workers = 0;
    config->mail.dkim_private = NULL;
    config->mail.dkim_selector = "";
    config->mail.host = "";
    config->migrations.source_directory = "";
    config->document = NULL;
    config->servers = NULL;
    config->mimetypes = NULL;
    config->databases = NULL;
    config->storages = NULL;

    return config;
}

int __config_load(config_t* config, const char *path) {
    if (path == NULL) return 0;

    int fd = open(path, O_RDONLY);
    if (fd == -1) return 0;

    char* data = helpers_read_file(fd);

    close(fd);

    if (data == NULL) return 0;

    int result = 0;

    if (!__config_parse_data(config, data)) goto failed;

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

    config_t* config = __config_create();
    if (config == NULL) return 0;

    config_t* oldconfig =_config;

    __config_load(config, _config_path);

    _config = config;

    config_free(oldconfig);

    return 1;
}

int config_init(int argc, char* argv[]) {
    config_t* config = __config_create();
    if (config == NULL) return 0;

    int result = 0;

    const char* config_path = __config_get_path(argc, argv);
    if (config_path == NULL) {
        log_print("Usage: -c <path to config file>\n", "");
        goto failed;
    }

    if (__config_load(config, config_path) == -1) {
        log_print("Can't load config\n", "");
        goto failed;
    }

    if (!__config_save_path(config_path)) {
        log_print("Can't save config path\n", "");
        goto failed;
    }

    _config = config;

    result = 1;

    failed:

    if (!result) {
        config_free(config);
    }

    return result;
}

void config_free(config_t* config) {
    json_free(config->document);

    if (config->mail.dkim_private != NULL) {
        free(config->mail.dkim_private);
        config->mail.dkim_private = NULL;
    }

    free(config);
}

int __config_parse_data(config_t* config, const char* data) {
    jsondoc_t* document = json_init();
    if (document == NULL) return 0;

    config->document = document;

    if (!json_parse(document, data)) return 0;

    const jsontok_t* root = json_root(document);
    if (!json_is_object(root)) return 0;

    if (!__config_fill_struct(config))
        return 0;

    return 1;
}

const config_t* config() {
    return _config;
}

void config_set(const config_t* config) {
    _config = (config_t*)config;
}

int __config_fill_struct(config_t* config) {
    const jsontok_t* root = json_root(config->document);

    const jsontok_t* token_migrations = json_object_get(root, "migrations");
    if (token_migrations == NULL) {
        config->migrations.source_directory = "";
    }
    else {
        if (!json_is_object(token_migrations)) return 0;

        const jsontok_t* token_source_directory = json_object_get(token_migrations, "source_directory");
        if (!json_is_string(token_source_directory)) return 0;
        config->migrations.source_directory = json_string(token_source_directory);
    }

    const jsontok_t* token_main = json_object_get(root, "main");
    if (!json_is_object(token_main)) return 0;

    const jsontok_t* token_workers = json_object_get(token_main, "workers");
    if (!json_is_int(token_workers)) return 0;
    config->main.workers = json_uint(token_workers);

    const jsontok_t* token_threads = json_object_get(token_main, "threads");
    if (!json_is_int(token_threads)) return 0;
    config->main.threads = json_uint(token_threads);

    const jsontok_t* token_reload = json_object_get(token_main, "reload");
    if (!json_is_string(token_reload)) return 0;
    config->main.reload = (strcmp(json_string(token_reload), "hard") == 0)
        ? CONFIG_RELOAD_HARD
        : CONFIG_RELOAD_SOFT;

    const jsontok_t* token_read_buffer = json_object_get(token_main, "read_buffer");
    if (!json_is_int(token_read_buffer)) return 0;
    config->main.read_buffer = json_uint(token_read_buffer);

    const jsontok_t* token_client_max_body_size = json_object_get(token_main, "client_max_body_size");
    if (!json_is_int(token_client_max_body_size)) return 0;
    config->main.client_max_body_size = json_uint(token_client_max_body_size);

    const jsontok_t* token_tmp = json_object_get(token_main, "tmp");
    if (!json_is_string(token_tmp)) return 0;
    config->main.tmp = json_string(token_tmp);
    const size_t tmp_length = strlen(config->main.tmp);
    if (config->main.tmp[tmp_length - 1] == '/') {
        log_print("[Error][Config] Remove last slash from main.tmp\n");
        return 0;
    }

    const jsontok_t* token_gzip = json_object_get(token_main, "gzip");
    if (!json_is_array(token_gzip)) return 0;
    config->main.gzip = token_gzip;


    const jsontok_t* token_servers = json_object_get(root, "servers");
    if (!json_is_object(token_servers)) return 0;
    config->servers = token_servers;

    const jsontok_t* token_databases = json_object_get(root, "databases");
    if (!json_is_object(token_databases)) return 0;
    config->databases = token_databases;

    const jsontok_t* token_storages = json_object_get(root, "storages");
    if (!json_is_object(token_storages)) return 0;
    config->storages = token_storages;

    const jsontok_t* token_mimetypes = json_object_get(root, "mimetypes");
    if (!json_is_object(token_mimetypes)) return 0;
    config->mimetypes = token_mimetypes;



    const jsontok_t* token_mail = json_object_get(root, "mail");
    if (!json_is_object(token_mail)) return 0;

    const jsontok_t* token_dkim_private = json_object_get(token_mail, "dkim_private");
    if (!json_is_string(token_dkim_private)) return 0;
    const char* dkim_private = json_string(token_dkim_private);

    file_t file = file_open(dkim_private, O_RDONLY);
    if (!file.ok) return 0;

    config->mail.dkim_private = file.content(&file);

    file.close(&file);

    if (config->mail.dkim_private == NULL)
        return 0;

    const jsontok_t* token_dkim_selector = json_object_get(token_mail, "dkim_selector");
    if (!json_is_string(token_dkim_selector)) return 0;
    config->mail.dkim_selector = json_string(token_dkim_selector);

    const jsontok_t* token_host = json_object_get(token_mail, "host");
    if (!json_is_string(token_host)) return 0;
    config->mail.host = json_string(token_host);

    return 1;
}
