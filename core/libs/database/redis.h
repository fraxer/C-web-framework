#ifndef __REDIS__
#define __REDIS__

#include <hiredis/hiredis.h>

#include "json.h"
#include "database.h"

typedef struct redishost {
    dbhost_t base;
    int dbindex;
    char* user;
    char* password;
} redishost_t;

typedef struct redisconnection {
    dbconnection_t base;
    redisContext* connection;
} redisconnection_t;

redishost_t* redis_host_create(void);
void redis_host_free(void* arg);
dbconnection_t* redis_connection_create(dbhosts_t* hosts);
db_t* redis_load(const char* database_id, const jsontok_t* token_array);

#endif