#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>

#include "log.h"
#include "appconfig.h"

static char* __appconfig_path = NULL;
static appconfig_t* __appconfig = NULL;
static void(*__appconfig_after_run_threads_cb)(void) = NULL;

static const char* __appconfig_get_path(int argc, char* argv[]);
static void __appconfig_env_init(env_t* env);
static void __appconfig_env_free(env_t* env);
static void __appconfig_env_gzip_free(env_gzip_str_t* item);

int appconfig_init(int argc, char* argv[]) {
    appconfig_t* config = appconfig_create(__appconfig_get_path(argc, argv));
    if (config == NULL) return 0;

    __appconfig_path = strdup(config->path);
    if (__appconfig_path == NULL) {
        appconfig_free(config);
        return 0;
    }

    appconfig_set(config);

    return 1;
}

appconfig_t* appconfig_create(const char* path) {
    if (path == NULL) return NULL;

    appconfig_t* config = malloc(sizeof * config);
    if (config == NULL) return NULL;

    atomic_store(&config->shutdown, 0);
    atomic_store(&config->threads_pause, 1);
    atomic_store(&config->threads_pause_lock, 0);
    atomic_store(&config->threads_stop_count, 0);
    atomic_store(&config->threads_count, 0);
    __appconfig_env_init(&config->env);
    config->mimetype = NULL;
    config->databases = NULL;
    config->storages = NULL;
    config->viewstore = NULL;
    config->server_chain = NULL;
    memset(&config->sessionconfig, 0, sizeof(config->sessionconfig));
    config->path = strdup(path);
    if (config->path == NULL) {
        log_print("Usage: -c <path to config file>\n", "");
        free(config);
        return NULL;
    }

    return config;
}

appconfig_t* appconfig(void) {
    return __appconfig;
}

env_t* env(void) {
    return &__appconfig->env;
}

void appconfig_set(appconfig_t* config) {
    __appconfig = config;
}

void appconfig_clear(appconfig_t* config) {
    if (config == NULL) return;

    atomic_store(&config->shutdown, 0);
    atomic_store(&config->threads_pause, 1);
    atomic_store(&config->threads_pause_lock, 0);
    atomic_store(&config->threads_stop_count, 0);
    atomic_store(&config->threads_count, 0);
    __appconfig_env_free(&config->env);

    mimetype_destroy(config->mimetype);
    config->mimetype = NULL;

    array_free(config->databases);
    config->databases = NULL;

    storages_free(config->storages);
    config->storages = NULL;

    viewstore_destroy(config->viewstore);
    config->viewstore = NULL;

    server_chain_destroy(config->server_chain);
    config->server_chain = NULL;

    sessionconfig_clear(&config->sessionconfig);
}

void appconfig_free(appconfig_t* config) {
    if (config == NULL) return;

    appconfig_clear(config);

    if (config->path != NULL) free(config->path);

    free(config);
}

char* appconfig_path(void) {
    return __appconfig_path;
}

void appconfig_lock(appconfig_t* config) {
    if (config == NULL) return;

    _Bool expected = 0;
    _Bool desired = 1;

    do {
        expected = 0;
    } while (!atomic_compare_exchange_strong(&config->threads_pause_lock, &expected, desired));
}

void appconfig_unlock(appconfig_t* config) {
    if (config == NULL) return;

    atomic_store(&config->threads_pause_lock, 0);
}

void appconfg_threads_wait(appconfig_t* config) {
    appconfig_lock(config);

    const int threads_count = config->env.main.threads + config->env.main.workers;
    if (atomic_load(&config->threads_count) == threads_count) {
        atomic_store(&config->threads_pause, 0);
        appconfig_unlock(config);
        __appconfig_after_run_threads_cb();
        return;
    }

    appconfig_unlock(config);

    while (atomic_load(&config->threads_pause))
        usleep(1000);
}

void appconfg_threads_increment(appconfig_t* config) {
    atomic_fetch_add(&config->threads_count, 1);
}

void appconfg_threads_decrement(appconfig_t* config) {
    atomic_fetch_sub(&config->threads_count, 1);

    if (atomic_load(&config->threads_count) == 0) {
        if (__appconfig == config)
            __appconfig = NULL;

        appconfig_free(config);
    }
}

void __appconfig_env_init(env_t* env) {
    if (env == NULL) return;

    env->main.reload = APPCONFIG_RELOAD_SOFT;
    env->main.client_max_body_size = 0;
    env->main.gzip = NULL;
    env->main.buffer_size = 0;
    env->main.threads = 0;
    env->main.workers = 0;
    env->main.tmp = NULL;
    env->mail.dkim_private = NULL;
    env->mail.dkim_selector = NULL;
    env->mail.host = NULL;
    env->migrations.source_directory = NULL;
}

void __appconfig_env_free(env_t* env) {
    if (env == NULL) return;

    env->main.client_max_body_size = 0;
    env->main.buffer_size = 0;
    env->main.threads = 0;
    env->main.workers = 0;

    if (env->main.gzip != NULL) {
        __appconfig_env_gzip_free(env->main.gzip);
        env->main.gzip = NULL;
    }

    if (env->main.tmp != NULL) {
        free(env->main.tmp);
        env->main.tmp = NULL;
    }

    if (env->mail.dkim_private != NULL) {
        free(env->mail.dkim_private);
        env->mail.dkim_private = NULL;
    }

    if (env->mail.dkim_selector != NULL) {
        free(env->mail.dkim_selector);
        env->mail.dkim_selector = NULL;
    }

    if (env->mail.host != NULL) {
        free(env->mail.host);
        env->mail.host = NULL;
    }

    if (env->migrations.source_directory != NULL) {
        free(env->migrations.source_directory);
        env->migrations.source_directory = NULL;
    }
}

void __appconfig_env_gzip_free(env_gzip_str_t* item) {
    while (item != NULL) {
        env_gzip_str_t* next = item->next;
        free(item->mimetype);
        free(item);
        item = next;
    }
}

const char* __appconfig_get_path(int argc, char* argv[]) {
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
                if (optarg == NULL)
                    return NULL;

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

void appconfig_set_after_run_threads_cb(void (*appconfig_after_run_threads_cb)(void)) {
    __appconfig_after_run_threads_cb = appconfig_after_run_threads_cb;
}