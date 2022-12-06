    #include <stdio.h>

void* view(void* request) {
    printf("run view handler, %s\n", (char*)request);
    return 0;
}

void* user(void* request) {
    printf("run user handler, %s\n", (char*)request);
    return 0;
}