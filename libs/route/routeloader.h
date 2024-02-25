#ifndef __ROUTELOADER__
#define __ROUTELOADER__

#define ROUTELOADER_LIB_NOT_FOUND "Route loader: Library not found \"%s\"\n"
#define ROUTELOADER_FUNCTION_NOT_FOUND "Route loader: Function \"%s\" not found in \"%s\"\n"
#define ROUTELOADER_OUT_OF_MEMORY "Route loader: Out of memory\n"

typedef struct routeloader_lib {
    char* filepath;
    void* pointer;

    struct routeloader_lib* next;
} routeloader_lib_t;

routeloader_lib_t* routeloader_load_lib(const char*);

void* routeloader_get_handler_silent(routeloader_lib_t*, const char*, const char*);
void* routeloader_get_handler(routeloader_lib_t*, const char*, const char*);

void routeloader_free(routeloader_lib_t*);

int routeloader_has_lib(routeloader_lib_t*, const char*);

routeloader_lib_t* routeloader_get_last(routeloader_lib_t*);

#endif
