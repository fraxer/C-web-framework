#ifndef __MIMETYPE__
#define __MIMETYPE__

#include <search.h>

typedef struct hsearch_data hsearch_data_t;

void mimetype_lock();

void mimetype_unlock();

int mimetype_init_ext(size_t);

int mimetype_init_type(size_t);

void mimetype_destroy(hsearch_data_t*);

hsearch_data_t* mimetype_get_table_ext();

hsearch_data_t* mimetype_get_table_type();

int mimetype_add(hsearch_data_t*, const char*, const char*);

const char* mimetype_find_ext(const char*);

const char* mimetype_find_type(const char*);

#endif
