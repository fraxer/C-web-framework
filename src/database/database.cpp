#include <cstdlib>
#include <cstring>
#include <cstdio>
#include "database.h"

namespace database {

namespace {

struct database_t {
    char* host;
    short port;
    char* dbname;
    short connections_timeout;
    connection_t* connections;
    char* user;
    char* password;
};

} // namespace

// typedef setParam_func_t int (*)(const char*, void*);

// setParam_func_t init() {

int init() {

    if (database != nullptr) {
        free(database);
    }

    database = (database_t*)malloc(sizeof(database_t));

    if (database == nullptr) {
        // log error
        return -1;
    }

    return 0;
}

} // namespace
