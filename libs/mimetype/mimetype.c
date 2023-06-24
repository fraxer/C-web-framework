#define _GNU_SOURCE
#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

#include "mimetype.h"

static hsearch_data_t* table_ext = NULL;
static hsearch_data_t* table_type = NULL;
static mimetypelist_t* list = NULL;
static mimetypelist_t* newlist = NULL;
static mimetypelist_t* last_list = NULL;

static atomic_bool flag = 0;

hsearch_data_t* mimetype_alloc();
hsearch_data_t* mimetype_create();
void mimetype_free(mimetypelist_t*);


void mimetype_lock() {
    _Bool expected = 0;
    _Bool desired = 1;

    if (atomic_compare_exchange_strong(&flag, &expected, desired)) return;
}

void mimetype_unlock() {
    atomic_store(&flag, 0);
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
    int result = -1;
    if (table == NULL) return -1;

    ENTRY item;
    ENTRY* res = NULL;

    mimetype_lock();

    mimetypelist_t* listitem_key = mimetype_get_item(mimetype);
    if (listitem_key == NULL) goto failed;

    item.key = listitem_key->value;

    mimetypelist_t* listitem_value = mimetype_get_item(extension);
    if (listitem_value == NULL) goto failed;

    item.data = (void*)listitem_value->value;

    if (hsearch_r(item, ENTER, &res, table) == 0) goto failed;

    result = 0;

    failed:

    if (result == -1) {
        mimetype_free(newlist);
    }

    mimetype_unlock();

    return result;
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

mimetypelist_t* mimetype_create_item(const char* string) {
    mimetypelist_t* item = malloc(sizeof * item);
    if (item == NULL) return NULL;

    item->value = malloc(strlen(string) + 1);
    if (item->value == NULL) {
        free(item);
        return NULL;
    }

    strcpy(item->value, string);

    item->next = NULL;

    mimetype_append_listitem(item);

    return item;
}

mimetypelist_t* mimetype_find_item(const char* string) {
    mimetypelist_t* item = newlist;

    while (item) {
        if (strcmp(item->value, string) == 0)
            return item;

        item = item->next;
    }

    return NULL;
}

mimetypelist_t* mimetype_get_item(const char* string) {
    mimetypelist_t* item = mimetype_find_item(string);

    if (item) return item;

    return mimetype_create_item(string);
}

void mimetype_append_listitem(mimetypelist_t* item) {
    if (last_list == NULL)
        newlist = item;
    else
        last_list->next = item;

    last_list = item;
}

void mimetype_free_listitem(mimetypelist_t* item) {
    if (item == NULL) return;

    if (item->value) free(item->value);

    item->value = NULL;
    item->next = NULL;

    free(item);
}

void mimetype_update() {
    mimetype_free(list);

    list = newlist;

    newlist = NULL;
    last_list = NULL;
}

void mimetype_free(mimetypelist_t* list) {
    while (list) {
        mimetypelist_t* next = list->next;

        mimetype_free_listitem(list);

        list = next;
    }
}
