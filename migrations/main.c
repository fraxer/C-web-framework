#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include "../src/database/dbquery.h"

char* config_get_path(int argc, char* argv[]) {
    for (int i = 0; i < argc; i++) {
        printf("%s\n", argv[i]);
    }

    if (argc < 3) {
        return NULL;
    }

    return NULL;
}

int main(int argc, char* argv[]) {
    char* ch = config_get_path(argc, argv);

    printf("asd");

    return EXIT_SUCCESS;
}