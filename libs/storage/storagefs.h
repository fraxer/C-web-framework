#ifndef __STORAGEFS__
#define __STORAGEFS__

#include "storage.h"

typedef struct {
    storage_t base;
    char root[2048];
} storagefs_t;

storagefs_t* storage_create_fs(const char* storage_name, const char* root);

#endif