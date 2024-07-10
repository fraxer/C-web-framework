#ifndef __USE_GNU
#define __USE_GNU
#endif

#ifndef __MIMETYPE__
#define __MIMETYPE__

#include <search.h>

typedef struct hsearch_data hsearch_data_t;

typedef struct {
    hsearch_data_t table_ext;
    hsearch_data_t table_type;
} mimetype_t;

mimetype_t* mimetype_create(size_t table_type_size, size_t table_ext_size);
void mimetype_destroy(mimetype_t* mimetype);
hsearch_data_t* mimetype_get_table_ext(mimetype_t* mimetype);
hsearch_data_t* mimetype_get_table_type(mimetype_t* mimetype);
int mimetype_add(hsearch_data_t* table, const char* key, const char* value);
const char* mimetype_find_ext(mimetype_t* mimetype, const char* key);
const char* mimetype_find_type(mimetype_t* mimetype, const char* key);

#endif
