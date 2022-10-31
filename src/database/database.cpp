#include <cstdlib>
#include <cstring>
    #include <cstdio>
#include "database.h"
#include "../helpers/helpers.h"

namespace database {

// typedef setParam_func_t int (*)(const char*, void*);

// setParam_func_t init() {

void* init() {

    if (database != nullptr) free();

    database = (database_t*)malloc(sizeof(database_t));

    if (database == nullptr) {
        // log error
        return nullptr;
    }

    return database;
}

void free() {

    helpers::freeNull(database->host);
    helpers::freeNull(database->dbname);
    helpers::freeNull(database->user);
    helpers::freeNull(database->password);

    database->driver = NONE;
    database->port = 0;
    database->connections_timeout = 0;

    // connection_t* connections;

    helpers::freeNull(database);

    return;
}

} // namespace
