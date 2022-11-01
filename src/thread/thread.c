#include <stddef.h>
#include "thread.h"
    #include <stdio.h>
    #include <unistd.h>

void* thread_init() {
    return NULL;
}

void* thread_worker(void* arg) {
    int i = 1;

    while (1) {
        sleep(1);

        if (i > 5) break;

        printf("thread_worker %d\n", i);

        i++;
    }
    
    
}

void* thread_handler(void* arg) {
    return NULL;
}
