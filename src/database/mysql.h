#ifndef __CMYSQL__
#define __CMYSQL__

#include <mysql.h>
#include "../jsmn/jsmn.h"
#include "database.h"

typedef struct myconfig {
    dbconfig_t base;
    dbhost_t* host;
    dbhost_t* current_host;
    char* dbname;
    char* user;
    char* password;
} myconfig_t;

typedef struct myconnection {
    dbconnection_t base;
    MYSQL* connection;
} myconnection_t;

myconfig_t* my_config_create();

void my_config_free(void*);

dbconnection_t* my_connection_create(dbconfig_t*);

const char* my_host(myconfig_t*);

int my_port(myconfig_t*);

void my_next_host(myconfig_t*);

db_t* my_load(const char*, const jsmntok_t* token_object);

#endif