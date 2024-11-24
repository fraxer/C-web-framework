#ifndef __APPCONFIG__
#define __APPCONFIG__

#include "array.h"
#include "server.h"
#include "storage.h"
#include "database.h"
#include "viewstore.h"
#include "mimetype.h"
#include "session.h"

typedef struct env_gzip_str {
    char* mimetype;
    struct env_gzip_str* next;
} env_gzip_str_t;

typedef enum {
    APPCONFIG_RELOAD_SOFT = 0,
    APPCONFIG_RELOAD_HARD
} appconfig_reload_state_e;

typedef struct env_main {
    appconfig_reload_state_e reload;
    unsigned int workers;
    unsigned int threads;
    unsigned int buffer_size;
    unsigned int client_max_body_size;
    char* tmp;
    env_gzip_str_t* gzip;
} env_main_t;

typedef struct env_mail {
    char* dkim_private;
    char* dkim_selector;
    char* host;
} env_mail_t;

typedef struct env_migrations {
    char* source_directory;
} env_migrations_t;

typedef struct {
    env_main_t main;
    env_mail_t mail;
    env_migrations_t migrations;
} env_t;

typedef struct appconfig {
    atomic_bool shutdown;
    atomic_bool threads_pause;
    atomic_bool threads_pause_lock;
    atomic_int threads_stop_count;
    atomic_int threads_count;
    env_t env;
    sessionconfig_t sessionconfig;
    char* path;
    mimetype_t* mimetype;
    array_t* databases;
    storage_t* storages;
    viewstore_t* viewstore;
    server_chain_t* server_chain;
} appconfig_t;

int appconfig_init(int argc, char* argv[]);
appconfig_t* appconfig_create(const char* path);
appconfig_t* appconfig(void);
env_t* env(void);
void appconfig_set(appconfig_t* config);
void appconfig_clear(appconfig_t* config);
void appconfig_free(appconfig_t* config);
char* appconfig_path(void);
void appconfig_lock(appconfig_t* config);
void appconfig_unlock(appconfig_t* config);
void appconfg_threads_wait(appconfig_t* config);
void appconfg_threads_increment(appconfig_t* config);
void appconfg_threads_decrement(appconfig_t* config);
void appconfig_set_after_run_threads_cb(void (*appconfig_after_run_threads_cb)(void));

#endif