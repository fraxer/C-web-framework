#ifndef __STORAGES3__
#define __STORAGES3__

#include "storage.h"

typedef struct {
    storage_t base;
    char access_id[128];
    char access_secret[128];
    char protocol[6];
    char host[128];
    char port[6];
    char bucket[128];
} storages3_t;

storages3_t* storage_create_s3(const char* storage_name, const char* access_id, const char* access_secret, const char* protocol, const char* host, const char* port, const char* bucket);

#endif