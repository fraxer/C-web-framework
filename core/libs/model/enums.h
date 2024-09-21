#ifndef __ENUMS__
#define __ENUMS__

typedef struct enums {
    int count;
    char** values;
} enums_t;

enums_t* enums_create(char** values, int count);
void enums_free(enums_t* enums);

#endif