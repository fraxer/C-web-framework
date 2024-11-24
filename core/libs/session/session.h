#ifndef __SESSION__
#define __SESSION__

#include <time.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <linux/limits.h>

typedef enum {
    SESSION_TYPE_NONE = 0,
    SESSION_TYPE_FS,
    SESSION_TYPE_REDIS,
} session_type_e;

typedef struct session {
    char*(*create)(const char* data);
    char*(*get)(const char* session_id);
    int(*update)(const char* session_id, const char* data);
    int(*destroy)(const char* session_id);
    void(*remove_expired)(void);
} session_t;

typedef struct sessionconfig {
    session_type_e driver;
    char storage_name[NAME_MAX];
    char host_id[NAME_MAX];
    int lifetime;
    session_t* session;
} sessionconfig_t;

char* session_create_id();
char* session_create(const char* data);
int session_destroy(const char* session_id);
int session_update(const char* session_id, const char* data);
char* session_get(const char* session_id);
void session_remove_expired(void);
void sessionconfig_clear(sessionconfig_t* sessionconfig);

session_t* sessionfile_init();
session_t* sessionredis_init();

#endif