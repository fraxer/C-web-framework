#ifndef __CONFIG__
#define __CONFIG__

#include "json.h"

typedef enum config_reload {
    CONFIG_RELOAD_SOFT = 0,
    CONFIG_RELOAD_HARD,
} config_reload_e;

typedef struct config_main {
    unsigned int workers;
    unsigned int threads;
    config_reload_e reload;
    unsigned int read_buffer;
    unsigned int client_max_body_size;
    const char* tmp;
    const jsontok_t* gzip;
} config_main_t;

typedef struct config_migrations {
    const char* source_directory;
} config_migrations_t;

typedef struct config {
    config_main_t main;
    config_migrations_t migrations;
    const jsontok_t* servers;
    const jsontok_t* mimetypes;
    const jsontok_t* databases;
    const jsontok_t* storages;
} config_t;

int config_reload();
int config_init(int argc, char* argv[]);
void config_free();

const config_t* config();
void config_set(const config_t*);

#endif