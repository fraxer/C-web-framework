#ifndef __CMYSQL__
#define __CMYSQL__

#include <mysql.h>
#include "../jsmn/jsmn.h"
#include "database.h"

typedef struct myhost {
    dbhost_t base;
    int port;
    char* ip;
    char* dbname;
    char* user;
    char* password;
} myhost_t;

typedef struct myconnection {
    dbconnection_t base;
    MYSQL* connection;
} myconnection_t;

myhost_t* my_host_create();

void my_host_free(void*);

dbconnection_t* my_connection_create(dbhosts_t*);

void my_next_host(dbhosts_t*);

db_t* my_load(const char*, const jsmntok_t*);

#endif