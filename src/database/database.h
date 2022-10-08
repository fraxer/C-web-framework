#ifndef __DATABASE__
#define __DATABASE__

namespace database {

namespace {

enum transaction_level {
    READ_UNCOMMITTED,
    READ_COMMITTED,
    REPEATABLE_READ,
    SERIALIZABLE
};

struct connection_t {};

struct database_t;

database_t* database = nullptr;

} // namespace

int init();

// void initConnections();

int query();

void begin();

void begin(transaction_level);

void commit();

void rollback();

} // namespace

#endif