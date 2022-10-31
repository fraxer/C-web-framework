#ifndef __DATABASE__
#define __DATABASE__

namespace database {

enum driver_e {
    NONE,
    POSTGRESQL,
    MYSQL,
};

namespace {

enum transaction_level_e {
    READ_UNCOMMITTED,
    READ_COMMITTED,
    REPEATABLE_READ,
    SERIALIZABLE
};

struct connection_t {
    char* host;
};

struct database_t {
    char* host;
    unsigned short port;
    char* dbname;
    char* user;
    char* password;
    driver_e driver;
    unsigned short connections_timeout;
    connection_t* connections;
};

} // namespace

database_t* database = nullptr;

void* init();

// void initConnections();

int query();

void begin();

void begin(transaction_level_e);

void commit();

void rollback();

void free();

} // namespace

#endif