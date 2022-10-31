#include <dlfcn.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include "../log/log.h"
#include "routeloader.h"
    #include <stdio.h>

typedef struct routeloader_lib {
    char* filepath;
    void* pointer;

    struct routeloader_lib* next;
} routeloader_lib_t;

static routeloader_lib_t* first_lib = NULL;
static routeloader_lib_t* last_lib = NULL;

routeloader_lib_t* routeloader_init_container(const char*, void*);

int routeloader_has_lib(const char*);

int routeloader_load_lib(const char* filepath) {
    void* shared_lib_p = dlopen(filepath, RTLD_LAZY);

    if (shared_lib_p == NULL) {
        log_error(ROUTELOADER_LIB_NOT_FOUND, filepath);
        return -1;
    }

    if (routeloader_has_lib(filepath)) return 0;

    routeloader_lib_t* routeloader_lib = routeloader_init_container(filepath, shared_lib_p);

    if (routeloader_lib == NULL) {
        log_error(ROUTELOADER_OUT_OF_MEMORY);
        return -1;
    }

    if (first_lib == NULL) {
        first_lib = routeloader_lib;
    }

    if (last_lib == NULL) {
        last_lib = routeloader_lib;
    } else {
        last_lib->next = routeloader_lib;
        last_lib = last_lib->next;
    }

    return 0;
}

void* routeloader_get_handler(const char* filepath, const char* function_name) {
    routeloader_lib_t* lib = first_lib;

    while (lib) {
        if (strcmp(lib->filepath, filepath) == 0) {
            void* function_p;

            *(void**)(&function_p) = dlsym(lib->pointer, function_name);

            if (function_p == NULL) {
                log_error(ROUTELOADER_FUNCTION_NOT_FOUND, function_name, filepath);
                return NULL;
            }

            return function_p;
        }

        lib = lib->next;
    }

    return NULL;
}

void routeloader_free() {
    routeloader_lib_t* lib = first_lib;

    while (lib) {
        routeloader_lib_t* next = lib->next;

        free(lib->filepath);

        dlclose(lib->pointer);

        free(lib);

        lib = next;
    }
}

routeloader_lib_t* routeloader_init_container(const char* filepath, void* shared_lib) {
    routeloader_lib_t* routeloader_lib = (routeloader_lib_t*)malloc(sizeof(routeloader_lib));

    if (routeloader_lib == NULL) return NULL;

    routeloader_lib->filepath = (char*)malloc(strlen(filepath) + 1);

    if (routeloader_lib->filepath == NULL) return NULL;

    strcpy(routeloader_lib->filepath, filepath);

    routeloader_lib->pointer = shared_lib;
    routeloader_lib->next = NULL;

    return routeloader_lib;
}

int routeloader_has_lib(const char* filepath) {
    routeloader_lib_t* lib = first_lib;

    while (lib) {
        if (strcmp(lib->filepath, filepath) == 0) return 1;

        lib = lib->next;
    }

    return 0;
}