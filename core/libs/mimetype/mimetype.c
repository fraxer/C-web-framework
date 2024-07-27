#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "mimetype.h"

typedef struct _ENTRY {
    unsigned int used;
    ENTRY entry;
} _ENTRY;

static void __mimetype_table_clear(hsearch_data_t* table);

mimetype_t* mimetype_create(size_t table_type_size, size_t table_ext_size) {
    table_type_size = (table_type_size + 1) * 1.5;
    table_ext_size = (table_ext_size + 1) * 1.5;

    mimetype_t* mimetype = malloc(sizeof * mimetype);
    if (mimetype == NULL) return NULL;

    mimetype->table_type.filled = 0;
    mimetype->table_type.size = 0;
    mimetype->table_type.table = NULL;
    mimetype->table_ext.filled = 0;
    mimetype->table_ext.size = 0;
    mimetype->table_ext.table = NULL;

    if (hcreate_r(table_type_size, &mimetype->table_type) == 0) {
        free(mimetype);
        return NULL;
    }

    if (hcreate_r(table_ext_size, &mimetype->table_ext) == 0) {
        hdestroy_r(&mimetype->table_type);
        free(mimetype);
        return NULL;
    }

    return mimetype;
}

void mimetype_destroy(mimetype_t* mimetype) {
    if (mimetype == NULL) return;

    __mimetype_table_clear(&mimetype->table_ext);
    __mimetype_table_clear(&mimetype->table_type);

    hdestroy_r(&mimetype->table_ext);
    hdestroy_r(&mimetype->table_type);

    free(mimetype);
}

void __mimetype_table_clear(hsearch_data_t* table) {
    if (table == NULL) return;

    for (size_t i = 0; i < table->size; i++) {
        _ENTRY* e = &table->table[i];
        if (e->used != 0) {
            if (e->entry.key != NULL) free(e->entry.key);
            if (e->entry.data != NULL) free(e->entry.data);
        }
    }
}

hsearch_data_t* mimetype_get_table_ext(mimetype_t* mimetype) {
    return &mimetype->table_ext;
}

hsearch_data_t* mimetype_get_table_type(mimetype_t* mimetype) {
    return &mimetype->table_type;
}

int mimetype_add(hsearch_data_t* table, const char* key, const char* value) {
    int result = 0;
    if (table == NULL) return 0;

    ENTRY item = {
        .key = NULL,
        .data = NULL
    };
    item.key = malloc(sizeof(char) * (strlen(key) + 1));
    if (item.key == NULL) {
        log_error("mimetype_add: can't alloc memory for key failed\n");
        goto failed;
    }

    strcpy(item.key, key);

    item.data = malloc(sizeof(char) * (strlen(value) + 1));
    if (item.data == NULL) {
        log_error("mimetype_add: can't alloc memory for value failed\n");
        goto failed;
    }

    strcpy(item.data, value);

    ENTRY* res = NULL;
    if (hsearch_r(item, ENTER, &res, table) == 0) goto failed;

    result = 1;

    failed:

    if (result == 0) {
        if (item.key != NULL) free(item.key);
        if (item.data != NULL) free(item.data);
    }

    return result;
}

const char* mimetype_find_ext(mimetype_t* mimetype, const char* key) {
    if (mimetype == NULL) return NULL;
    if (key == NULL) return NULL;

    const char* result = NULL;

    ENTRY item = {
        .key = (char*)key,
        .data = NULL
    };
    ENTRY* res = NULL;

    if (hsearch_r(item, FIND, &res, &mimetype->table_type) == 0) goto failed;

    result = res->data;

    failed:

    return result;
}

const char* mimetype_find_type(mimetype_t* mimetype, const char* key) {
    if (mimetype == NULL) return NULL;
    if (key == NULL) return NULL;

    const char* result = NULL;

    ENTRY item = {
        .key = (char*)key,
        .data = NULL
    };
    ENTRY* res = NULL;

    if (hsearch_r(item, FIND, &res, &mimetype->table_ext) == 0) goto failed;

    result = res->data;

    failed:

    return result;
}
