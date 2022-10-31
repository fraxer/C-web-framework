#ifndef __ROUTELOADER__
#define __ROUTELOADER__

#define ROUTELOADER_LIB_NOT_FOUND "Route loader: Library not found \"%s\"\n"
#define ROUTELOADER_FUNCTION_NOT_FOUND "Route loader: Function \"%s\" not found in \"%s\"\n"
#define ROUTELOADER_OUT_OF_MEMORY "Route loader: Out of memory\n"

int routeloader_load_lib(const char*);

void* routeloader_get_handler(const char*, const char*);

void routeloader_free();

#endif
