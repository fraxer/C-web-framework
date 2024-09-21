#include <stdlib.h>
#include <string.h>

#include "enums.h"

enums_t* enums_create(char** values, int count) {
    enums_t* enums = malloc(sizeof * enums);
    if (enums == NULL) return NULL;

    enums->count = count;
    enums->values = malloc(sizeof(char*) * count);
    if (enums->values == NULL) {
        free(enums);
        return NULL;
    }

    for (int i = 0; i < count; i++)
        enums->values[i] = NULL;

    for (int i = 0; i < count; i++) {
        enums->values[i] = strdup(values[i]);
        if (enums->values[i] == NULL) {
            enums_free(enums);
            return NULL;
        }
    }

    return enums;
}

void enums_free(enums_t* enums) {
    if (enums == NULL) return;
    if (enums->values == NULL) {
        free(enums);
        return;
    }

    for (int i = 0; i < enums->count; i++)
        if (enums->values[i] != NULL)
            free(enums->values[i]);

    free(enums->values);
    free(enums);
}
