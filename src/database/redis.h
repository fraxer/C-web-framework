#ifndef __REDIS__
#define __REDIS__

#include <hiredis/hiredis.h>
#include "../jsmn/jsmn.h"
#include "database.h"

typedef struct redishost {
    dbhost_t base;
    int port;
    int dbindex;
    char* ip;
    char* user;
    char* password;
} redishost_t;

typedef struct redisconnection {
    dbconnection_t base;
    redisContext* connection;
} redisconnection_t;

redishost_t* redis_host_create();

void redis_host_free(void*);

dbconnection_t* redis_connection_create(dbhosts_t*);

void redis_next_host(dbhosts_t*);

db_t* redis_load(const char*, const jsmntok_t*);

#endif