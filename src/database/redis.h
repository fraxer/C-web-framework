#ifndef __REDIS__
#define __REDIS__

#include <hiredis/hiredis.h>
#include "../jsmn/jsmn.h"
#include "database.h"

typedef struct redisconfig {
    dbconfig_t base;
    dbhost_t* host;
    dbhost_t* current_host;
    int dbindex;
    char* user;
    char* password;
} redisconfig_t;

typedef struct redisconnection {
    dbconnection_t base;
    redisContext* connection;
} redisconnection_t;

redisconfig_t* redis_config_create();

void redis_config_free(void*);

dbconnection_t* redis_connection_create(dbconfig_t*);

const char* redis_host(redisconfig_t*);

int redis_port(redisconfig_t*);

void redis_next_host(redisconfig_t*);

db_t* redis_load(const char*, const jsmntok_t* token_object);

#endif