#define _GNU_SOURCE
#include <pthread.h>
#include <stdatomic.h>
#include <search.h>
#include <stdlib.h>
#include "mimetype.h"

static hsearch_data_t* table_ext = NULL;
static hsearch_data_t* table_type = NULL;

static atomic_flag flag = ATOMIC_FLAG_INIT;

hsearch_data_t* mimetype_alloc();
hsearch_data_t* mimetype_create();


void mimetype_lock() {
    while (atomic_flag_test_and_set(&flag));
}

void mimetype_unlock() {
    atomic_flag_clear(&flag);
}

int mimetype_init_ext(size_t size) {
    if (table_ext != NULL) mimetype_destroy(table_ext);

    table_ext = mimetype_create();

    if (table_ext == NULL) return -1;

    if (hcreate_r(size, table_ext) == 0) return -1;

    return 0;
}

int mimetype_init_type(size_t size) {
    if (table_type != NULL) mimetype_destroy(table_type);

    table_type = mimetype_create();

    if (table_type == NULL) return -1;

    if (hcreate_r(size, table_type) == 0) return -1;

    return 0;
}

void mimetype_destroy(hsearch_data_t* table) {
    if (table == NULL) return;

    hdestroy_r(table);

    free(table);

    table = NULL;
}

hsearch_data_t* mimetype_alloc() {
    return (hsearch_data_t*)malloc(sizeof(hsearch_data_t));
}

hsearch_data_t* mimetype_create() {
    hsearch_data_t* table = mimetype_alloc();

    if (table == NULL) return NULL;

    table->table = NULL;
    table->size = 0;
    table->filled = 0;

    return table;
}

hsearch_data_t* mimetype_get_table_ext() {
    return table_ext;
}

hsearch_data_t* mimetype_get_table_type() {
    return table_type;
}

int mimetype_add(hsearch_data_t* table, const char* mimetype, const char* extension) {
    if (table == NULL) return -1;

    ENTRY item;
    ENTRY* result = NULL;

    item.key = (char*)mimetype;
    item.data = (void*)extension;

    if (hsearch_r(item, ENTER, &result, table) == 0) return -1;

    return 0;
}

const char* mimetype_find_ext(const char* mimetype) {
    if (mimetype == NULL) return NULL;
    if (table_type == NULL) return NULL;

    const char* result = NULL;

    ENTRY item;
    ENTRY* res = NULL;

    item.key = (char*)mimetype;
    item.data = NULL;

    mimetype_lock();

    if (hsearch_r(item, FIND, &res, table_type) == 0) goto failed;

    result = res->data;

    failed:

    mimetype_unlock();

    return result;
}

const char* mimetype_find_type(const char* extension) {
    if (extension == NULL) return NULL;
    if (table_ext == NULL) return NULL;

    const char* result = NULL;

    ENTRY item;
    ENTRY* res = NULL;

    item.key = (char*)extension;
    item.data = NULL;

    mimetype_lock();

    if (hsearch_r(item, FIND, &res, table_ext) == 0) goto failed;

    result = res->data;

    failed:

    mimetype_unlock();

    return result;
}